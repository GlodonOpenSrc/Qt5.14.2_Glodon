#include <algorithm>
#include <vector>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "perfetto/ext/base/file_utils.h"
#include "perfetto/ext/base/scoped_file.h"
#include "perfetto/protozero/field.h"
#include "perfetto/protozero/proto_decoder.h"
#include "perfetto/protozero/proto_utils.h"
#include "third_party/pprof/profile.pb.h"

namespace perfetto {
namespace protoprofile {
namespace {

using protozero::proto_utils::ProtoWireType;
using GLine = ::perftools::profiles::Line;
using GMapping = ::perftools::profiles::Mapping;
using GLocation = ::perftools::profiles::Location;
using GProfile = ::perftools::profiles::Profile;
using GValueType = ::perftools::profiles::ValueType;
using GFunction = ::perftools::profiles::Function;
using GSample = ::perftools::profiles::Sample;
using ::google::protobuf::Descriptor;
using ::google::protobuf::DynamicMessageFactory;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::FileDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::compiler::DiskSourceTree;
using ::google::protobuf::compiler::Importer;
using ::google::protobuf::compiler::MultiFileErrorCollector;

class MultiFileErrorCollectorImpl : public MultiFileErrorCollector {
 public:
  ~MultiFileErrorCollectorImpl() override;
  void AddError(const std::string& filename,
                int line,
                int column,
                const std::string& message) override;

  void AddWarning(const std::string& filename,
                  int line,
                  int column,
                  const std::string& message) override;
};

MultiFileErrorCollectorImpl::~MultiFileErrorCollectorImpl() = default;

void MultiFileErrorCollectorImpl::AddError(const std::string& filename,
                                           int line,
                                           int column,
                                           const std::string& message) {
  PERFETTO_ELOG("Error %s %d:%d: %s", filename.c_str(), line, column,
                message.c_str());
}

void MultiFileErrorCollectorImpl::AddWarning(const std::string& filename,
                                             int line,
                                             int column,
                                             const std::string& message) {
  PERFETTO_ELOG("Error %s %d:%d: %s", filename.c_str(), line, column,
                message.c_str());
}

class SizeProfileComputer {
 public:
  GProfile Compute(const uint8_t* ptr,
                   size_t size,
                   const Descriptor* descriptor);

 private:
  struct StackInfo {
    std::vector<size_t> samples;
    std::vector<int> locations;
  };

  void ComputeInner(const uint8_t* ptr,
                    size_t size,
                    const Descriptor* descriptor);
  void Sample(size_t size);
  int InternString(const std::string& str);
  int InternLocation(const std::string& str);
  size_t GetFieldSize(const protozero::Field& f);

  // Convert the current stack into a string:
  // {"Foo", "#bar", "Bar", "#baz", "int"} -> "Foo$#bar$Bar$#baz$int$"
  std::string StackToKey();

  // The current 'stack' we're considering as we parse the protobuf.
  // For example if we're currently looking at the varint field baz which is
  // nested inside message Bar which is in turn a field named bar on the message
  // Foo. Then the stack would be: Foo, #bar, Bar, #baz, int
  // We keep track of both the field names (#bar, #baz) and the field types
  // (Foo, Bar, int) as sometimes we are intrested in which fields are big
  // and sometimes which types are big.
  std::vector<std::string> stack_;

  // Information about each stack seen. Keyed by a unique string for each stack.
  std::map<std::string, StackInfo> stack_info_;

  // Interned strings:
  std::vector<std::string> strings_;
  std::map<std::string, int> string_to_id_;

  // Interned 'locations', each location is a single frame of the stack.
  std::map<std::string, int> locations_;
};

size_t SizeProfileComputer::GetFieldSize(const protozero::Field& f) {
  uint8_t buf[10];
  switch (f.type()) {
    case protozero::proto_utils::ProtoWireType::kVarInt:
      return static_cast<size_t>(
          protozero::proto_utils::WriteVarInt(f.as_uint64(), buf) - buf);
    case protozero::proto_utils::ProtoWireType::kLengthDelimited:
      return f.size();
    case protozero::proto_utils::ProtoWireType::kFixed32:
      return 4;
    case protozero::proto_utils::ProtoWireType::kFixed64:
      return 8;
  }
  PERFETTO_FATAL("unexpected field type");  // for gcc
}

int SizeProfileComputer::InternString(const std::string& s) {
  if (string_to_id_.count(s)) {
    return string_to_id_[s];
  }
  strings_.push_back(s);
  int id = static_cast<int>(strings_.size() - 1);
  string_to_id_[s] = id;
  return id;
}

int SizeProfileComputer::InternLocation(const std::string& s) {
  if (locations_.count(s)) {
    return locations_[s];
  }
  int id = static_cast<int>(locations_.size()) + 1;
  locations_[s] = id;
  return id;
}

std::string SizeProfileComputer::StackToKey() {
  std::string key;
  for (const std::string& part : stack_) {
    key += part;
    key += "$";
  }
  return key;
}

void SizeProfileComputer::Sample(size_t size) {
  const std::string& key = StackToKey();

  if (!stack_info_.count(key)) {
    StackInfo& info = stack_info_[key];
    info.locations.resize(stack_.size());
    for (size_t i = 0; i < stack_.size(); i++) {
      info.locations[i] = InternLocation(stack_[i]);
    }
  }

  stack_info_[key].samples.push_back(size);
}

GProfile SizeProfileComputer::Compute(const uint8_t* ptr,
                                      size_t size,
                                      const Descriptor* descriptor) {
  PERFETTO_CHECK(InternString("") == 0);
  ComputeInner(ptr, size, descriptor);
  GProfile profile;

  GValueType* sample_type;

  sample_type = profile.add_sample_type();
  sample_type->set_type(InternString("protos"));
  sample_type->set_unit(InternString("count"));

  sample_type = profile.add_sample_type();
  sample_type->set_type(InternString("max_size"));
  sample_type->set_unit(InternString("bytes"));

  sample_type = profile.add_sample_type();
  sample_type->set_type(InternString("min_size"));
  sample_type->set_unit(InternString("bytes"));

  sample_type = profile.add_sample_type();
  sample_type->set_type(InternString("median"));
  sample_type->set_unit(InternString("bytes"));

  sample_type = profile.add_sample_type();
  sample_type->set_type(InternString("total_size"));
  sample_type->set_unit(InternString("bytes"));

  // For each unique stack we've seen write out the stats:
  for (auto& id_info : stack_info_) {
    StackInfo& info = id_info.second;

    GSample* sample = profile.add_sample();
    for (auto it = info.locations.rbegin(); it != info.locations.rend(); ++it) {
      sample->add_location_id(static_cast<uint64_t>(*it));
    }

    std::sort(info.samples.begin(), info.samples.end());
    size_t count = info.samples.size();
    size_t total_size = 0;
    size_t max_size = info.samples[count - 1];
    size_t min_size = info.samples[0];
    size_t median_size = info.samples[count / 2];
    for (size_t i = 0; i < count; ++i)
      total_size += info.samples[i];
    // These have to be in the same order as the sample types above:
    sample->add_value(static_cast<int64_t>(count));
    sample->add_value(static_cast<int64_t>(max_size));
    sample->add_value(static_cast<int64_t>(min_size));
    sample->add_value(static_cast<int64_t>(median_size));
    sample->add_value(static_cast<int64_t>(total_size));
  }

  // The proto profile has a two step mapping where samples are associated with
  // locations which in turn are associated to functions. We don't currently
  // distinguish them so we make a 1:1 mapping between the locations and the
  // functions:
  for (const auto& location_id : locations_) {
    GLocation* location = profile.add_location();
    location->set_id(static_cast<uint64_t>(location_id.second));
    GLine* line = location->add_line();
    line->set_function_id(static_cast<uint64_t>(location_id.second));
  }

  for (const auto& location_id : locations_) {
    GFunction* function = profile.add_function();
    function->set_id(static_cast<uint64_t>(location_id.second));
    function->set_name(InternString(location_id.first));
  }

  // Finally the string table. We intern more strings above, so this has to be
  // last.
  for (int i = 0; i < static_cast<int>(strings_.size()); i++) {
    profile.add_string_table(strings_[static_cast<size_t>(i)]);
  }
  return profile;
}

void SizeProfileComputer::ComputeInner(const uint8_t* ptr,
                                       size_t size,
                                       const Descriptor* descriptor) {
  size_t overhead = size;
  size_t unknown = 0;
  protozero::ProtoDecoder decoder(ptr, size);

  stack_.push_back(descriptor->name());

  // Compute the size of each sub-field of this message, subtracting it
  // from overhead and possible adding it to unknown.
  for (;;) {
    if (decoder.bytes_left() == 0)
      break;
    protozero::Field field = decoder.ReadField();
    if (!field.valid()) {
      PERFETTO_ELOG("Field not valid (can mean field id >1000)");
      break;
    }

    int id = field.id();
    ProtoWireType type = field.type();
    size_t field_size = GetFieldSize(field);

    overhead -= field_size;
    const FieldDescriptor* field_descriptor = descriptor->FindFieldByNumber(id);
    if (!field_descriptor) {
      unknown += field_size;
      continue;
    }

    stack_.push_back("#" + field_descriptor->name());
    bool is_message_type =
        field_descriptor->type() == FieldDescriptor::TYPE_MESSAGE;
    if (type == ProtoWireType::kLengthDelimited && is_message_type) {
      ComputeInner(field.data(), field.size(),
                   field_descriptor->message_type());
    } else {
      stack_.push_back(field_descriptor->type_name());
      Sample(field_size);
      stack_.pop_back();
    }
    stack_.pop_back();
  }

  if (unknown) {
    stack_.push_back("#:unknown:");
    Sample(unknown);
    stack_.pop_back();
  }

  // Anything not blamed on a child is overhead for this message.
  Sample(overhead);
  stack_.pop_back();
}

int PrintUsage(int, const char** argv) {
  fprintf(stderr, "Usage: %s INPUT_PATH OUTPUT_PATH\n", argv[0]);
  return 1;
}

int Main(int argc, const char** argv) {
  if (argc != 3)
    return PrintUsage(argc, argv);

  const char* input_path = argv[1];
  const char* output_path = argv[2];

  base::ScopedFile proto_fd = base::OpenFile(input_path, O_RDONLY);
  if (!proto_fd) {
    PERFETTO_ELOG("Could not open input path (%s)", input_path);
    return 1;
  }

  std::string s;
  base::ReadFileDescriptor(proto_fd.get(), &s);

  const Descriptor* descriptor;
  DiskSourceTree dst;
  dst.MapPath("perfetto", "protos/perfetto");
  MultiFileErrorCollectorImpl mfe;
  Importer importer(&dst, &mfe);
  const FileDescriptor* parsed_file =
      importer.Import("perfetto/trace/trace.proto");
  DynamicMessageFactory dmf;
  descriptor = parsed_file->message_type(0);

  const uint8_t* start = reinterpret_cast<const uint8_t*>(s.data());
  size_t size = s.size();

  if (!descriptor) {
    PERFETTO_ELOG("Could not parse trace.proto");
    return 1;
  }

  base::ScopedFile output_fd =
      base::OpenFile(output_path, O_WRONLY | O_TRUNC | O_CREAT, 0600);
  if (!output_fd) {
    PERFETTO_ELOG("Could not open output path (%s)", output_path);
    return 1;
  }
  SizeProfileComputer computer;
  GProfile profile = computer.Compute(start, size, descriptor);
  std::string out;
  profile.SerializeToString(&out);
  base::WriteAll(output_fd.get(), out.data(), out.size());
  base::FlushFile(output_fd.get());

  return 0;
}

}  // namespace
}  // namespace protoprofile
}  // namespace perfetto

int main(int argc, const char** argv) {
  return perfetto::protoprofile::Main(argc, argv);
}
