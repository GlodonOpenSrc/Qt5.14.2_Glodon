// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/system_connector.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_switches.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/video_capture/public/cpp/mock_producer.h"
#include "services/video_capture/public/mojom/constants.mojom.h"
#include "services/video_capture/public/mojom/device_factory.mojom.h"
#include "services/video_capture/public/mojom/device_factory_provider.mojom.h"
#include "services/video_capture/public/mojom/devices_changed_observer.mojom.h"
#include "services/video_capture/public/mojom/producer.mojom.h"
#include "services/video_capture/public/mojom/video_source_provider.mojom.h"
#include "services/video_capture/public/mojom/virtual_device.mojom.h"

namespace content {

// The mediadevices.ondevicechange event is currently not supported on Android.
#if defined(OS_ANDROID)
#define MAYBE_AddingAndRemovingVirtualDeviceTriggersMediaElementOnDeviceChange \
  DISABLED_AddingAndRemovingVirtualDeviceTriggersMediaElementOnDeviceChange
#else
#define MAYBE_AddingAndRemovingVirtualDeviceTriggersMediaElementOnDeviceChange \
  AddingAndRemovingVirtualDeviceTriggersMediaElementOnDeviceChange
#endif

namespace {

enum class ServiceApi { kSingleClient, kMultiClient };
enum class VirtualDeviceType { kSharedMemory, kTexture };

struct TestParams {
  ServiceApi api_to_use;
  VirtualDeviceType device_type;
};

static const char kVideoCaptureHtmlFile[] = "/media/video_capture_test.html";
static const char kEnumerateVideoCaptureDevicesAndVerify[] =
    "enumerateVideoCaptureDevicesAndVerifyCount(%d)";
static const char kRegisterForDeviceChangeEvent[] =
    "registerForDeviceChangeEvent()";
static const char kWaitForDeviceChangeEvent[] = "waitForDeviceChangeEvent()";
static const char kResetHasReceivedChangedEventFlag[] =
    "resetHasReceivedChangedEventFlag()";

}  // anonymous namespace

// Integration test that obtains a connection to the video capture service via
// the Browser process' service manager. It then registers and unregisters
// virtual devices at the service and checks in JavaScript that the list of
// enumerated devices changes correspondingly.
class WebRtcVideoCaptureServiceEnumerationBrowserTest
    : public ContentBrowserTest,
      public testing::WithParamInterface<TestParams>,
      public video_capture::mojom::DevicesChangedObserver {
 public:
  WebRtcVideoCaptureServiceEnumerationBrowserTest()
      : devices_changed_observer_binding_(this) {
    scoped_feature_list_.InitAndEnableFeature(features::kMojoVideoCapture);
  }

  ~WebRtcVideoCaptureServiceEnumerationBrowserTest() override {}

  void ConnectToService() {
    connector_->BindInterface(video_capture::mojom::kServiceName, &provider_);
    video_capture::mojom::DevicesChangedObserverPtr observer;
    devices_changed_observer_binding_.Bind(mojo::MakeRequest(&observer));
    switch (GetParam().api_to_use) {
      case ServiceApi::kSingleClient:
        provider_->ConnectToDeviceFactory(mojo::MakeRequest(&factory_));
        factory_->RegisterVirtualDevicesChangedObserver(
            std::move(observer),
            false /*raise_event_if_virtual_devices_already_present*/);
        break;
      case ServiceApi::kMultiClient:
        provider_->ConnectToVideoSourceProvider(
            mojo::MakeRequest(&video_source_provider_));
        video_source_provider_->RegisterVirtualDevicesChangedObserver(
            std::move(observer),
            false /*raise_event_if_virtual_devices_already_present*/);
        break;
    }
  }

  void AddVirtualDevice(const std::string& device_id) {
    media::VideoCaptureDeviceInfo info;
    info.descriptor.device_id = device_id;
    info.descriptor.set_display_name(device_id);
    info.descriptor.capture_api = media::VideoCaptureApi::VIRTUAL_DEVICE;

    base::RunLoop wait_loop;
    closure_to_be_called_on_devices_changed_ = wait_loop.QuitClosure();
    switch (GetParam().device_type) {
      case VirtualDeviceType::kSharedMemory: {
        video_capture::mojom::SharedMemoryVirtualDevicePtr virtual_device;
        video_capture::mojom::ProducerPtr producer;
        auto mock_producer = std::make_unique<video_capture::MockProducer>(
            mojo::MakeRequest(&producer));
        switch (GetParam().api_to_use) {
          case ServiceApi::kSingleClient:
            factory_->AddSharedMemoryVirtualDevice(
                info, std::move(producer), false,
                mojo::MakeRequest(&virtual_device));
            break;
          case ServiceApi::kMultiClient:
            video_source_provider_->AddSharedMemoryVirtualDevice(
                info, std::move(producer), false,
                mojo::MakeRequest(&virtual_device));
            break;
        }
        shared_memory_devices_by_id_.insert(std::make_pair(
            device_id, std::make_pair(std::move(virtual_device),
                                      std::move(mock_producer))));
        break;
      }
      case VirtualDeviceType::kTexture: {
        video_capture::mojom::TextureVirtualDevicePtr virtual_device;
        switch (GetParam().api_to_use) {
          case ServiceApi::kSingleClient:
            factory_->AddTextureVirtualDevice(
                info, mojo::MakeRequest(&virtual_device));
            break;
          case ServiceApi::kMultiClient:
            video_source_provider_->AddTextureVirtualDevice(
                info, mojo::MakeRequest(&virtual_device));
            break;
        }
        texture_devices_by_id_.insert(
            std::make_pair(device_id, std::move(virtual_device)));
        break;
      }
    }
    // Wait for confirmation from the service.
    wait_loop.Run();
  }

  void RemoveVirtualDevice(const std::string& device_id) {
    base::RunLoop wait_loop;
    closure_to_be_called_on_devices_changed_ = wait_loop.QuitClosure();
    switch (GetParam().device_type) {
      case VirtualDeviceType::kSharedMemory:
        shared_memory_devices_by_id_.erase(device_id);
        break;
      case VirtualDeviceType::kTexture:
        texture_devices_by_id_.erase(device_id);
        break;
    }

    // Wait for confirmation from the service.
    wait_loop.Run();
  }

  void DisconnectFromService() {
    factory_ = nullptr;
    video_source_provider_ = nullptr;
    provider_ = nullptr;
  }

  void EnumerateDevicesInRendererAndVerifyDeviceCount(
      int expected_device_count) {
    const std::string javascript_to_execute = base::StringPrintf(
        kEnumerateVideoCaptureDevicesAndVerify, expected_device_count);
    std::string result;
    ASSERT_TRUE(
        ExecuteScriptAndExtractString(shell(), javascript_to_execute, &result));
    ASSERT_EQ("OK", result);
  }

  void RegisterForDeviceChangeEventInRenderer() {
    ASSERT_TRUE(ExecuteScript(shell(), kRegisterForDeviceChangeEvent));
  }

  void WaitForDeviceChangeEventInRenderer() {
    std::string result;
    ASSERT_TRUE(ExecuteScriptAndExtractString(
        shell(), kWaitForDeviceChangeEvent, &result));
    ASSERT_EQ("OK", result);
  }

  void ResetHasReceivedChangedEventFlag() {
    ASSERT_TRUE(ExecuteScript(shell(), kResetHasReceivedChangedEventFlag));
  }

  // Implementation of video_capture::mojom::DevicesChangedObserver:
  void OnDevicesChanged() override {
    if (closure_to_be_called_on_devices_changed_) {
      std::move(closure_to_be_called_on_devices_changed_).Run();
    }
  }

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Note: We are not planning to actually use any fake device, but we want
    // to avoid enumerating or otherwise calling into real capture devices.
    command_line->AppendSwitchASCII(switches::kUseFakeDeviceForMediaStream,
                                    "device-count=0");
    command_line->AppendSwitch(switches::kUseFakeUIForMediaStream);
  }

  void Initialize() {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
    embedded_test_server()->StartAcceptingConnections();

    NavigateToURL(shell(),
                  GURL(embedded_test_server()->GetURL(kVideoCaptureHtmlFile)));

    auto* connector = GetSystemConnector();
    ASSERT_TRUE(connector);
    connector_ = connector->Clone();
  }

  std::unique_ptr<service_manager::Connector> connector_;
  std::map<std::string, video_capture::mojom::TextureVirtualDevicePtr>
      texture_devices_by_id_;
  std::map<std::string,
           std::pair<video_capture::mojom::SharedMemoryVirtualDevicePtr,
                     std::unique_ptr<video_capture::MockProducer>>>
      shared_memory_devices_by_id_;

 private:
  mojo::Binding<video_capture::mojom::DevicesChangedObserver>
      devices_changed_observer_binding_;
  base::test::ScopedFeatureList scoped_feature_list_;
  video_capture::mojom::DeviceFactoryProviderPtr provider_;
  video_capture::mojom::DeviceFactoryPtr factory_;
  video_capture::mojom::VideoSourceProviderPtr video_source_provider_;
  base::OnceClosure closure_to_be_called_on_devices_changed_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcVideoCaptureServiceEnumerationBrowserTest);
};

IN_PROC_BROWSER_TEST_P(WebRtcVideoCaptureServiceEnumerationBrowserTest,
                       SingleAddedVirtualDeviceGetsEnumerated) {
  Initialize();
  ConnectToService();

  // Exercise
  AddVirtualDevice("test");
  EnumerateDevicesInRendererAndVerifyDeviceCount(1);

  // Tear down
  RemoveVirtualDevice("test");
  DisconnectFromService();
}

IN_PROC_BROWSER_TEST_P(WebRtcVideoCaptureServiceEnumerationBrowserTest,
                       RemoveVirtualDeviceAfterItHasBeenEnumerated) {
  Initialize();
  ConnectToService();

  AddVirtualDevice("test_1");
  AddVirtualDevice("test_2");
  EnumerateDevicesInRendererAndVerifyDeviceCount(2);
  RemoveVirtualDevice("test_1");
  EnumerateDevicesInRendererAndVerifyDeviceCount(1);
  RemoveVirtualDevice("test_2");
  EnumerateDevicesInRendererAndVerifyDeviceCount(0);

  // Tear down
  DisconnectFromService();
}

IN_PROC_BROWSER_TEST_P(
    WebRtcVideoCaptureServiceEnumerationBrowserTest,
    MAYBE_AddingAndRemovingVirtualDeviceTriggersMediaElementOnDeviceChange) {
  Initialize();
  ConnectToService();
  RegisterForDeviceChangeEventInRenderer();

  // Exercise
  AddVirtualDevice("test");

  WaitForDeviceChangeEventInRenderer();
  ResetHasReceivedChangedEventFlag();

  RemoveVirtualDevice("test");
  WaitForDeviceChangeEventInRenderer();

  // Tear down
  DisconnectFromService();
}

INSTANTIATE_TEST_SUITE_P(
    ,
    WebRtcVideoCaptureServiceEnumerationBrowserTest,
    ::testing::Values(
        TestParams{ServiceApi::kSingleClient, VirtualDeviceType::kSharedMemory},
        TestParams{ServiceApi::kSingleClient, VirtualDeviceType::kTexture},
        TestParams{ServiceApi::kMultiClient, VirtualDeviceType::kSharedMemory},
        TestParams{ServiceApi::kMultiClient, VirtualDeviceType::kTexture}));

}  // namespace content
