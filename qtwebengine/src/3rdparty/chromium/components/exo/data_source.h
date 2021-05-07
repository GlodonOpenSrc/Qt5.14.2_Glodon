// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_SOURCE_H_
#define COMPONENTS_EXO_DATA_SOURCE_H_

#include <string>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace exo {

class DataSourceDelegate;
class DataSourceObserver;
enum class DndAction;

// Object representing transferred data offered by a client.
class DataSource {
 public:
  explicit DataSource(DataSourceDelegate* delegate);
  ~DataSource();

  void AddObserver(DataSourceObserver* observer);
  void RemoveObserver(DataSourceObserver* observer);

  // Notifies to DataSource that the client offers new mime type.
  void Offer(const std::string& mime_type);

  // Notifies the possible drag and drop actions selected by the data source to
  // DataSource.
  void SetActions(const base::flat_set<DndAction>& dnd_actions);

  // Notifies the data source is cancelled. e.g. Replaced with another data
  // source.
  void Cancelled();

  // Search the set of offered MIME types for the most preferred of each of the
  // following categories: text/plain*, text/rtf, text/html*, image/*. If any
  // usable MIME types in a given category are available, the corresponding
  // |*_reader| input callback will be called with the best one and the
  // corresponding data. For any category that has no available MIME types,
  // |failure_callback| is run. |failure_callback| may therefore be run as many
  // as four times.
  using ReadDataCallback =
      base::OnceCallback<void(const std::string&, const std::vector<uint8_t>&)>;
  void GetDataForPreferredMimeTypes(ReadDataCallback text_reader,
                                    ReadDataCallback rtf_reader,
                                    ReadDataCallback html_reader,
                                    ReadDataCallback image_reader,
                                    base::RepeatingClosure failure_callback);

  void ReadDataForTesting(const std::string& mime_type,
                          ReadDataCallback callback);

 private:
  // Reads data from the source. Then |callback| is invoked with read data. If
  // Cancelled() is invoked or DataSource is destroyed before completion,
  // |callback| is never called, and |failure_callback| is run instead.
  void ReadData(const std::string& mime_type,
                ReadDataCallback callback,
                base::OnceClosure failure_callback);

  void OnDataRead(ReadDataCallback callback,
                  const std::string& mime_type,
                  const std::vector<uint8_t>&);

  DataSourceDelegate* const delegate_;
  base::ObserverList<DataSourceObserver>::Unchecked observers_;

  // Mime types which has been offered.
  std::set<std::string> mime_types_;
  bool cancelled_;

  base::WeakPtrFactory<DataSource> read_data_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DataSource);
};

class ScopedDataSource {
 public:
  ScopedDataSource(DataSource* data_source, DataSourceObserver* observer);
  ~ScopedDataSource();
  DataSource* get() { return data_source_; }

 private:
  DataSource* const data_source_;
  DataSourceObserver* const observer_;

  DISALLOW_COPY_AND_ASSIGN(ScopedDataSource);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_SOURCE_H_