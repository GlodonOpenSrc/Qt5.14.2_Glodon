// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/camera_hal_delegate.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/capture/video/chromeos/mock_camera_module.h"
#include "media/capture/video/chromeos/mock_vendor_tag_ops.h"
#include "media/capture/video/chromeos/video_capture_device_factory_chromeos.h"
#include "media/capture/video/mock_gpu_memory_buffer_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::A;
using testing::Invoke;
using testing::Return;

namespace {

constexpr uint32_t kDevicePathTag = 0x80000000;
constexpr char kFakeDevicePath[] = "/dev/video5566";

}  // namespace

namespace media {

class CameraHalDelegateTest : public ::testing::Test {
 public:
  CameraHalDelegateTest() : hal_delegate_thread_("HalDelegateThread") {}

  void SetUp() override {
    VideoCaptureDeviceFactoryChromeOS::SetGpuBufferManager(
        &mock_gpu_memory_buffer_manager_);
    hal_delegate_thread_.Start();
    camera_hal_delegate_ =
        new CameraHalDelegate(hal_delegate_thread_.task_runner());
    camera_hal_delegate_->SetCameraModule(
        mock_camera_module_.GetInterfacePtrInfo());
  }

  void TearDown() override {
    camera_hal_delegate_->Reset();
    hal_delegate_thread_.Stop();
  }

  void Wait() {
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<CameraHalDelegate> camera_hal_delegate_;
  testing::StrictMock<unittest_internal::MockCameraModule> mock_camera_module_;
  testing::StrictMock<unittest_internal::MockVendorTagOps> mock_vendor_tag_ops_;
  unittest_internal::MockGpuMemoryBufferManager mock_gpu_memory_buffer_manager_;

 private:
  base::Thread hal_delegate_thread_;
  std::unique_ptr<base::RunLoop> run_loop_;
  DISALLOW_COPY_AND_ASSIGN(CameraHalDelegateTest);
};

TEST_F(CameraHalDelegateTest, GetBuiltinCameraInfo) {
  auto get_number_of_cameras_cb =
      [](cros::mojom::CameraModule::GetNumberOfCamerasCallback& cb) {
        std::move(cb).Run(2);
      };

  auto get_camera_info_cb = [](uint32_t camera_id,
                               cros::mojom::CameraModule::GetCameraInfoCallback&
                                   cb) {
    cros::mojom::CameraInfoPtr camera_info = cros::mojom::CameraInfo::New();
    cros::mojom::CameraMetadataPtr static_metadata =
        cros::mojom::CameraMetadata::New();
    static_metadata->entry_count = 1;
    static_metadata->entry_capacity = 1;
    static_metadata->entries =
        std::vector<cros::mojom::CameraMetadataEntryPtr>();

    cros::mojom::CameraMetadataEntryPtr entry =
        cros::mojom::CameraMetadataEntry::New();
    entry->index = 0;
    entry->tag = cros::mojom::CameraMetadataTag::
        ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS;
    entry->type = cros::mojom::EntryType::TYPE_INT64;
    entry->count = 8;
    std::vector<int64_t> min_frame_durations(8);
    min_frame_durations[0] = static_cast<int64_t>(
        cros::mojom::HalPixelFormat::HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
    min_frame_durations[1] = 1280;
    min_frame_durations[2] = 720;
    min_frame_durations[3] = 33333333;
    min_frame_durations[4] = static_cast<int64_t>(
        cros::mojom::HalPixelFormat::HAL_PIXEL_FORMAT_YCbCr_420_888);
    min_frame_durations[5] = 1280;
    min_frame_durations[6] = 720;
    min_frame_durations[7] = 16666666;
    uint8_t* as_int8 = reinterpret_cast<uint8_t*>(min_frame_durations.data());
    entry->data.assign(as_int8, as_int8 + entry->count * sizeof(int64_t));
    static_metadata->entries->push_back(std::move(entry));

    switch (camera_id) {
      case 0:
        camera_info->facing = cros::mojom::CameraFacing::CAMERA_FACING_BACK;
        camera_info->orientation = 0;
        camera_info->static_camera_characteristics = std::move(static_metadata);
        break;
      case 1:
        camera_info->facing = cros::mojom::CameraFacing::CAMERA_FACING_FRONT;
        camera_info->orientation = 0;
        camera_info->static_camera_characteristics = std::move(static_metadata);
        break;
      case 2:
        entry = cros::mojom::CameraMetadataEntry::New();
        entry->index = static_metadata->entry_count;
        entry->tag =
            static_cast<cros::mojom::CameraMetadataTag>(kDevicePathTag);
        entry->type = cros::mojom::EntryType::TYPE_BYTE;
        entry->count = sizeof(kFakeDevicePath);
        entry->data.assign(std::begin(kFakeDevicePath),
                           std::end(kFakeDevicePath));

        static_metadata->entry_count++;
        static_metadata->entry_capacity++;
        static_metadata->entries->push_back(std::move(entry));

        camera_info->facing = cros::mojom::CameraFacing::CAMERA_FACING_EXTERNAL;
        camera_info->orientation = 0;
        camera_info->static_camera_characteristics = std::move(static_metadata);
        break;
      default:
        FAIL() << "Invalid camera id";
    }
    std::move(cb).Run(0, std::move(camera_info));
  };

  auto get_vendor_tag_ops_cb =
      [&](cros::mojom::VendorTagOpsRequest& vendor_tag_ops_request,
          cros::mojom::CameraModule::GetVendorTagOpsCallback&) {
        mock_vendor_tag_ops_.Bind(std::move(vendor_tag_ops_request));
      };

  auto set_callbacks_cb =
      [&](cros::mojom::CameraModuleCallbacksPtr& callbacks,
          cros::mojom::CameraModule::SetCallbacksCallback&) {
        mock_camera_module_.NotifyCameraDeviceChange(
            2, cros::mojom::CameraDeviceStatus::CAMERA_DEVICE_STATUS_PRESENT);
      };

  EXPECT_CALL(mock_camera_module_, DoGetNumberOfCameras(_))
      .Times(1)
      .WillOnce(Invoke(get_number_of_cameras_cb));
  EXPECT_CALL(
      mock_camera_module_,
      DoSetCallbacks(A<cros::mojom::CameraModuleCallbacksPtr&>(),
                     A<cros::mojom::CameraModule::SetCallbacksCallback&>()))
      .Times(1)
      .WillOnce(Invoke(set_callbacks_cb));
  EXPECT_CALL(mock_camera_module_,
              DoGetVendorTagOps(
                  A<cros::mojom::VendorTagOpsRequest&>(),
                  A<cros::mojom::CameraModule::GetVendorTagOpsCallback&>()))
      .Times(1)
      .WillOnce(Invoke(get_vendor_tag_ops_cb));
  EXPECT_CALL(mock_camera_module_,
              DoGetCameraInfo(
                  0, A<cros::mojom::CameraModule::GetCameraInfoCallback&>()))
      .Times(1)
      .WillOnce(Invoke(get_camera_info_cb));
  EXPECT_CALL(mock_camera_module_,
              DoGetCameraInfo(
                  1, A<cros::mojom::CameraModule::GetCameraInfoCallback&>()))
      .Times(1)
      .WillOnce(Invoke(get_camera_info_cb));
  EXPECT_CALL(mock_camera_module_,
              DoGetCameraInfo(
                  2, A<cros::mojom::CameraModule::GetCameraInfoCallback&>()))
      .Times(1)
      .WillOnce(Invoke(get_camera_info_cb));

  EXPECT_CALL(mock_vendor_tag_ops_, DoGetTagCount())
      .Times(1)
      .WillOnce(Return(1));

  EXPECT_CALL(mock_vendor_tag_ops_, DoGetAllTags())
      .Times(1)
      .WillOnce(Return(std::vector<uint32_t>{kDevicePathTag}));

  EXPECT_CALL(mock_vendor_tag_ops_, DoGetSectionName(kDevicePathTag))
      .Times(1)
      .WillOnce(Return("com.google"));

  EXPECT_CALL(mock_vendor_tag_ops_, DoGetTagName(kDevicePathTag))
      .Times(1)
      .WillOnce(Return("usb.devicePath"));

  EXPECT_CALL(mock_vendor_tag_ops_, DoGetTagType(kDevicePathTag))
      .Times(1)
      .WillOnce(
          Return(static_cast<int32_t>(cros::mojom::EntryType::TYPE_BYTE)));

  VideoCaptureDeviceDescriptors descriptors;
  camera_hal_delegate_->GetDeviceDescriptors(&descriptors);

  ASSERT_EQ(3u, descriptors.size());
  // We have workaround to always put front camera at first.
  ASSERT_EQ("1", descriptors[0].device_id);
  ASSERT_EQ(VideoFacingMode::MEDIA_VIDEO_FACING_USER, descriptors[0].facing);
  ASSERT_EQ("0", descriptors[1].device_id);
  ASSERT_EQ(VideoFacingMode::MEDIA_VIDEO_FACING_ENVIRONMENT,
            descriptors[1].facing);
  ASSERT_EQ(kFakeDevicePath, descriptors[2].device_id);
  ASSERT_EQ(VideoFacingMode::MEDIA_VIDEO_FACING_NONE, descriptors[2].facing);

  // TODO(shik): Test external camera. Check the fields |display_name| and
  // |model_id| are set properly according to the vendor tags.

  EXPECT_CALL(mock_gpu_memory_buffer_manager_,
              CreateGpuMemoryBuffer(_, gfx::BufferFormat::YUV_420_BIPLANAR,
                                    gfx::BufferUsage::SCANOUT_CAMERA_READ_WRITE,
                                    gpu::kNullSurfaceHandle))
      .Times(1)
      .WillOnce(Invoke(&unittest_internal::MockGpuMemoryBufferManager::
                           CreateFakeGpuMemoryBuffer));

  VideoCaptureFormats supported_formats;
  camera_hal_delegate_->GetSupportedFormats(descriptors[0], &supported_formats);

  // IMPLEMENTATION_DEFINED format should be filtered; currently YCbCr_420_888
  // format corresponds to NV12 in Chrome.
  ASSERT_EQ(1U, supported_formats.size());
  ASSERT_EQ(gfx::Size(1280, 720), supported_formats[0].frame_size);
  ASSERT_FLOAT_EQ(60.0, supported_formats[0].frame_rate);
  ASSERT_EQ(PIXEL_FORMAT_NV12, supported_formats[0].pixel_format);
}

}  // namespace media
