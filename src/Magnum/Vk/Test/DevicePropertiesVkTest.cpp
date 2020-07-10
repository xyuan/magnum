/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <sstream>
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/FormatStl.h>

#include "Magnum/Vk/DeviceProperties.h"
#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/ExtensionProperties.h"
#include "Magnum/Vk/Instance.h"
#include "Magnum/Vk/LayerProperties.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/Version.h"

namespace Magnum { namespace Vk { namespace Test { namespace {

struct DevicePropertiesVkTest: TestSuite::Tester {
    explicit DevicePropertiesVkTest();

    void enumerate();
    void constructMove();
    void wrap();

    /* Most of this tested already in ExtensionPropertiesVkTest, this only
       covers what isn't there already */
    void enumerateExtensions();
    void enumerateExtensionsWithKhronosValidationLayer();
    void enumerateExtensionsNonexistentLayer();
    void extensionIsSupported();

    void pickDevice();
    void pickDeviceIndex();
    void pickDeviceType();
    void pickDeviceError();

    Instance _instance;
};

const struct {
    const char* name;
    Containers::Array<const char*> args;
    const char* message;
} PickDeviceErrorData[] {
    {"nothing for type found", Containers::array({"", "--magnum-device", "virtual"}),
        "Vk::tryPickDevice(): no Vulkan device of type Vk::DeviceType::VirtualGpu found\n"},
    {"index out of bounds", Containers::array({"", "--magnum-device", "666"}),
        "Vk::tryPickDevice(): index 666 out of bounds for {} Vulkan devices\n"},
    {"unknown type", Containers::array({"", "--magnum-device", "FAST"}),
        "Vk::tryPickDevice(): unknown Vulkan device type FAST\n"}
};

DevicePropertiesVkTest::DevicePropertiesVkTest(): _instance{InstanceCreateInfo{arguments().first, arguments().second}} {
    addTests({&DevicePropertiesVkTest::enumerate,
              &DevicePropertiesVkTest::constructMove,
              &DevicePropertiesVkTest::wrap,

              &DevicePropertiesVkTest::enumerateExtensions,
              &DevicePropertiesVkTest::enumerateExtensionsWithKhronosValidationLayer,
              &DevicePropertiesVkTest::enumerateExtensionsNonexistentLayer,
              &DevicePropertiesVkTest::extensionIsSupported,

              &DevicePropertiesVkTest::pickDevice,
              &DevicePropertiesVkTest::pickDeviceIndex,
              &DevicePropertiesVkTest::pickDeviceType});

    addInstancedTests({&DevicePropertiesVkTest::pickDeviceError},
        Containers::arraySize(PickDeviceErrorData));
}

void DevicePropertiesVkTest::enumerate() {
    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    Debug{} << "Found" << devices.size() << "devices";
    CORRADE_VERIFY(!devices.empty());

    for(DeviceProperties& device: devices) {
        CORRADE_ITERATION(device.name());

        CORRADE_VERIFY(device.handle());
        CORRADE_COMPARE_AS(device.apiVersion(), Version::Vk10,
            TestSuite::Compare::GreaterOrEqual);
        CORRADE_COMPARE_AS(device.driverVersion(), Version::Vk10,
            TestSuite::Compare::GreaterOrEqual);
        CORRADE_VERIFY(device.type() != DeviceType::Other);
        CORRADE_VERIFY(!device.name().isEmpty());
    }
}

void DevicePropertiesVkTest::constructMove() {
    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    CORRADE_VERIFY(!devices.empty());
    VkPhysicalDevice handle = devices[0].handle();
    Containers::StringView name = devices[0].name();

    DeviceProperties a = std::move(devices[0]);
    CORRADE_COMPARE(a.handle(), handle);
    CORRADE_COMPARE(a.name(), name);

    DeviceProperties b = DeviceProperties::wrap(_instance, nullptr);
    b = std::move(a);
    CORRADE_COMPARE(b.handle(), handle);
    CORRADE_COMPARE(b.name(), name);
}

void DevicePropertiesVkTest::wrap() {
    VkPhysicalDevice handle;
    UnsignedInt count = 1;
    auto result = Result(_instance->EnumeratePhysicalDevices(_instance, &count, &handle));
    {
        /** @todo clean up once Compare::AnyOf exists */
        CORRADE_ITERATION(result);
        CORRADE_VERIFY(result == Result::Success || result == Result::Incomplete);
    }

    DeviceProperties wrapped = DeviceProperties::wrap(_instance, handle);
    CORRADE_VERIFY(wrapped.handle());

    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    CORRADE_COMPARE(wrapped.name(), devices[0].name());
}

void DevicePropertiesVkTest::enumerateExtensions() {
    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    CORRADE_VERIFY(!devices.empty());

    ExtensionProperties properties = devices[0].enumerateExtensionProperties();
    Debug{} << "Available device extension count:" << properties.names().size();

    CORRADE_COMPARE_AS(properties.count(), 0, TestSuite::Compare::Greater);

    /* The extension list should be sorted and unique (so Less, not
       LessOrEqual) */
    Containers::ArrayView<const Containers::StringView> extensions = properties.names();
    for(std::size_t i = 1; i != extensions.size(); ++i) {
        CORRADE_COMPARE_AS(extensions[i - 1], extensions[i],
            TestSuite::Compare::Less);
    }
}

void DevicePropertiesVkTest::enumerateExtensionsWithKhronosValidationLayer() {
    if(!enumerateLayerProperties().isSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");

    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    CORRADE_VERIFY(!devices.empty());

    /* There should be more extensions with this layer enabled */
    ExtensionProperties global = devices[0].enumerateExtensionProperties();
    ExtensionProperties withKhronosValidation = devices[0].enumerateExtensionProperties({"VK_LAYER_KHRONOS_validation"});
    CORRADE_COMPARE_AS(global.count(),
        withKhronosValidation.count(),
        TestSuite::Compare::Less);

    /* VK_EXT_tooling_info is only in the layer */
    CORRADE_VERIFY(!global.isSupported("VK_EXT_tooling_info"));
    CORRADE_VERIFY(withKhronosValidation.isSupported("VK_EXT_tooling_info"));
}

void DevicePropertiesVkTest::enumerateExtensionsNonexistentLayer() {
    CORRADE_SKIP("Currently this hits an internal assert, which can't be tested.");

    std::ostringstream out;
    Error redirectError{&out};
    enumerateInstanceExtensionProperties({"VK_LAYER_this_doesnt_exist"});
    CORRADE_COMPARE(out.str(), "TODO");
}

void DevicePropertiesVkTest::extensionIsSupported() {
    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    CORRADE_VERIFY(!devices.empty());

    ExtensionProperties properties = devices[0].enumerateExtensionProperties();

    /* This extension should be available almost always */
    if(!properties.isSupported("VK_KHR_maintenance1"))
        CORRADE_SKIP("VK_KHR_maintenance1 not supported, can't fully test");

    /* Verify the overloads that take our extension wrappers work as well */
    CORRADE_VERIFY(properties.isSupported<Extensions::KHR::maintenance1>());
    CORRADE_VERIFY(properties.isSupported(Extensions::KHR::maintenance1{}));
}

void DevicePropertiesVkTest::pickDevice() {
    /* Default behavior */
    Containers::Optional<DeviceProperties> device = tryPickDevice(_instance);
    CORRADE_VERIFY(device);
}

void DevicePropertiesVkTest::pickDeviceIndex() {
    Containers::Array<DeviceProperties> devices = enumerateDevices(_instance);
    CORRADE_VERIFY(!devices.empty());

    /* Pick the last one */
    CORRADE_COMPARE_AS(devices.size(), 10, TestSuite::Compare::Less);
    const char id[] {char('0' + devices.size() - 1), '\0'};
    const char* argv[] {"", "--magnum-device", id};

    /* Creating another dedicated instance so we can pass custom args */
    Instance instance{InstanceCreateInfo{Int(Containers::arraySize(argv)), argv}};

    Containers::Optional<DeviceProperties> device = tryPickDevice(instance);
    CORRADE_VERIFY(device);
}

void DevicePropertiesVkTest::pickDeviceType() {
    const char* argv[] {"", "--magnum-device", "cpu"};

    /* Creating a dedicated instance so we can pass custom args */
    Instance instance{InstanceCreateInfo{Int(Containers::arraySize(argv)), argv}};

    Containers::Optional<DeviceProperties> device = tryPickDevice(instance);
    if(!device) CORRADE_SKIP("No CPU device found.");

    CORRADE_VERIFY(device->type() == DeviceType::Cpu);
}

void DevicePropertiesVkTest::pickDeviceError() {
    auto&& data = PickDeviceErrorData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Creating a dedicated instance so we can pass custom args */
    Instance instance{InstanceCreateInfo{Int(data.args.size()), const_cast<const char**>(data.args.data())}};

    std::ostringstream out;
    Error redirectError{&out};
    CORRADE_VERIFY(!tryPickDevice(instance));
    CORRADE_COMPARE(out.str(), Utility::formatString(data.message, enumerateDevices(_instance).size()));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::DevicePropertiesVkTest)
