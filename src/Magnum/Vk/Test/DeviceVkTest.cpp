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
#include <Corrade/Containers/StringStl.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/FormatStl.h>

#include "Magnum/Vk/Device.h"
#include "Magnum/Vk/DeviceProperties.h"
#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/ExtensionProperties.h"
#include "Magnum/Vk/Instance.h"
#include "Magnum/Vk/LayerProperties.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/Version.h"

#include "MagnumExternal/Vulkan/flextVkGlobal.h"

namespace Magnum { namespace Vk { namespace Test { namespace {

struct DeviceVkTest: TestSuite::Tester {
    explicit DeviceVkTest();

    void createInfoConstruct();
    void createInfoConstructImplicitDevice();
    void createInfoConstructNoImplicitExtensions();
    void createInfoExtensions();
    void createInfoCopiedStrings();
    void createInfoNoPriorities();

    void construct();
    void constructExtensions();
    void constructCommandLineDisable();
    void constructCommandLineEnable();
    void constructMove();
    void constructUnknownExtension();
    void constructNoQueue();
    void wrap();
    void populateGlobalFunctionPointers();

    Instance _instance;
};

struct {
    const char* nameDisable;
    const char* nameEnable;
    Containers::Array<const char*> argsDisable, argsEnable;
    bool driverVersionSupported, debugMarkerEnabled, maintenance1Enabled;
    const char* log;
} ConstructCommandLineData[] {
    /* Shouldn't print anything about version, enabled layers/exts if verbose
       output isn't enabled. */
    {"", "enabled extensions", nullptr,
        Containers::array({"",
            "--magnum-enable-extensions", "VK_EXT_debug_marker VK_KHR_maintenance1"}),
        true, true, true,
        ""},
    /* Only with verbose log */
    {"verbose", "enabled extensions, verbose",
        Containers::array({"", "--magnum-log", "verbose"}),
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-enable-extensions", "VK_EXT_debug_marker VK_KHR_maintenance1"}),
        true, true, true,
        "Device version: Vulkan {}.{}.{}\n"
        "Enabled device extensions:\n"
        "    VK_EXT_debug_marker\n"
        "    VK_KHR_maintenance1\n"},
    {"forced version, verbose", "forced version, enabled extensions, verbose",
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-vulkan-version", "1.0"}),
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-vulkan-version", "1.0",
            "--magnum-enable-extensions", "VK_EXT_debug_marker VK_KHR_maintenance1"}),
        false, true, true,
        "Device version: Vulkan 1.0\n"
        "Enabled device extensions:\n"
        "    VK_EXT_debug_marker\n"
        "    VK_KHR_maintenance1\n"},
    {"disabled one extension", "enabled one extension",
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-disable-extensions", "VK_EXT_debug_marker"}),
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-enable-extensions", "VK_KHR_maintenance1"}),
        true, false, true,
        "Device version: Vulkan {}.{}.{}\n"
        "Enabled device extensions:\n"
        "    VK_KHR_maintenance1\n"},
    {"disabled extensions", "verbose",
        Containers::array({"", "--magnum-log", "verbose",
            "--magnum-disable-extensions", "VK_EXT_debug_marker VK_KHR_maintenance1"}),
        Containers::array({"", "--magnum-log", "verbose"}),
        true, false, false,
        "Device version: Vulkan {}.{}.{}\n"},
};

DeviceVkTest::DeviceVkTest(): _instance{InstanceCreateInfo{arguments().first, arguments().second}} {
    addTests({&DeviceVkTest::createInfoConstruct,
              &DeviceVkTest::createInfoConstructImplicitDevice,
              &DeviceVkTest::createInfoConstructNoImplicitExtensions,
              &DeviceVkTest::createInfoExtensions,
              &DeviceVkTest::createInfoCopiedStrings,
              &DeviceVkTest::createInfoNoPriorities,

              &DeviceVkTest::construct,
              &DeviceVkTest::constructExtensions});

    addInstancedTests({&DeviceVkTest::constructCommandLineDisable,
                       &DeviceVkTest::constructCommandLineEnable},
        Containers::arraySize(ConstructCommandLineData));

    addTests({&DeviceVkTest::constructMove,
              &DeviceVkTest::constructUnknownExtension,
              &DeviceVkTest::constructNoQueue,

              &DeviceVkTest::wrap,
              &DeviceVkTest::populateGlobalFunctionPointers});
}

using namespace Containers::Literals;

void DeviceVkTest::createInfoConstruct() {
    DeviceCreateInfo info{pickDevice(_instance)};
    CORRADE_VERIFY(info->sType);
    CORRADE_VERIFY(!info->pNext);
    /* Extensions might or might not be enabled */
}

void DeviceVkTest::createInfoConstructImplicitDevice() {
    DeviceCreateInfo info{_instance};
    CORRADE_VERIFY(info->sType);
    CORRADE_VERIFY(!info->pNext);
    /* Extensions might or might not be enabled */
}

void DeviceVkTest::createInfoConstructNoImplicitExtensions() {
    DeviceCreateInfo info{_instance, DeviceCreateInfo::Flag::NoImplicitExtensions};
    CORRADE_VERIFY(info->sType);
    CORRADE_VERIFY(!info->pNext);
    /* No extensions enabled as we explicitly disabled that */
    CORRADE_VERIFY(!info->ppEnabledExtensionNames);
    CORRADE_COMPARE(info->enabledExtensionCount, 0);
}

void DeviceVkTest::createInfoExtensions() {
    if(std::getenv("MAGNUM_DISABLE_EXTENSIONS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_EXTENSIONS environment variable set");

    DeviceCreateInfo info{_instance, DeviceCreateInfo::Flag::NoImplicitExtensions};
    CORRADE_VERIFY(!info->ppEnabledExtensionNames);
    CORRADE_COMPARE(info->enabledExtensionCount, 0);

    info.addEnabledExtensions<Extensions::KHR::maintenance1>();
    CORRADE_VERIFY(info->ppEnabledExtensionNames);
    CORRADE_COMPARE(info->enabledExtensionCount, 1);
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[0]),
        Extensions::KHR::maintenance1::string().data());

    info.addEnabledExtensions({
        Extensions::KHR::draw_indirect_count{},
        Extensions::KHR::get_memory_requirements2{}
    });
    CORRADE_COMPARE(info->enabledExtensionCount, 3);
    /* The pointer should be to the global data */
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[0]),
        Extensions::KHR::maintenance1::string().data());
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[1]),
        Extensions::KHR::draw_indirect_count::string().data());
    CORRADE_COMPARE(static_cast<const void*>(info->ppEnabledExtensionNames[2]),
        Extensions::KHR::get_memory_requirements2::string().data());
}

void DeviceVkTest::createInfoCopiedStrings() {
    Containers::StringView globalButNotNullTerminated = "VK_KHR_maintenance25"_s.except(1);
    Containers::String localButNullTerminated = Extensions::KHR::draw_indirect_count::string();

    DeviceCreateInfo info{_instance, DeviceCreateInfo::Flag::NoImplicitExtensions};
    info.addEnabledExtensions({
        globalButNotNullTerminated,
        localButNullTerminated
    });
    CORRADE_COMPARE(info->enabledExtensionCount, 2);

    CORRADE_COMPARE(info->ppEnabledExtensionNames[0], globalButNotNullTerminated);
    CORRADE_VERIFY(info->ppEnabledExtensionNames[0] != globalButNotNullTerminated.data());

    CORRADE_COMPARE(info->ppEnabledExtensionNames[1], localButNullTerminated);
    CORRADE_VERIFY(info->ppEnabledExtensionNames[1] != localButNullTerminated.data());
}

void DeviceVkTest::createInfoNoPriorities() {
    std::ostringstream out;
    Error redirectError{&out};
    DeviceCreateInfo{_instance}.addQueues(0, {});
    CORRADE_COMPARE(out.str(), "Vk::DeviceCreateInfo::addQueues(): at least one queue priority has to be specified\n");
}

void DeviceVkTest::construct() {
    if(std::getenv("MAGNUM_VULKAN_VERSION"))
        CORRADE_SKIP("Can't test with the MAGNUM_VULKAN_VERSION environment variable set");

    {
        DeviceProperties deviceProperties = pickDevice(_instance);
        Device device{_instance, DeviceCreateInfo{deviceProperties}
            .addQueues(0, {0.0f})
        };
        CORRADE_VERIFY(device.handle());
        /* Device function pointers should be populated */
        CORRADE_VERIFY(device->CreateBuffer);
        CORRADE_COMPARE(device.handleFlags(), HandleFlag::DestroyOnDestruction);
        CORRADE_COMPARE(device.version(), deviceProperties.apiVersion());
        /* Device version is supported */
        CORRADE_VERIFY(device.isVersionSupported(deviceProperties.apiVersion()));
        CORRADE_VERIFY(!device.isVersionSupported(Version::None));
        /* No extensions are enabled by default ... */
        CORRADE_VERIFY(!device.isExtensionEnabled<Extensions::EXT::debug_marker>());
        /* ... and thus also no function pointers loaded */
        CORRADE_VERIFY(!device->CmdDebugMarkerInsertEXT);
    }

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void DeviceVkTest::constructExtensions() {
    if(std::getenv("MAGNUM_DISABLE_EXTENSIONS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_EXTENSIONS environment variable set");

    if(!enumerateLayerProperties().isSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");

    /* Creating a dedicated instance so we can pass custom args and enable
       layers & exts independently */
    Instance instance{InstanceCreateInfo{}
        .addEnabledLayers({"VK_LAYER_KHRONOS_validation"})
        /* Needed by VK_EXT_debug_marker */
        .addEnabledExtensions<Extensions::EXT::debug_report>()
    };

    DeviceProperties deviceProperties = pickDevice(instance);
    ExtensionProperties extensions = deviceProperties.enumerateExtensionProperties({"VK_LAYER_KHRONOS_validation"});
    if(!extensions.isSupported<Extensions::EXT::debug_marker>())
        CORRADE_SKIP("VK_EXT_debug_marker not supported, can't test");
    if(!extensions.isSupported<Extensions::KHR::maintenance1>())
        CORRADE_SKIP("VK_KHR_maintenance1 not supported, can't test");

    Device device{instance, DeviceCreateInfo{deviceProperties}
        .addQueues(0, {0.0f})
        .addEnabledExtensions({
            Extensions::EXT::debug_marker::string(),
            "VK_KHR_maintenance1"_s
        })};
    CORRADE_VERIFY(device.handle());

    /* Extensions should be reported as enabled ... */
    CORRADE_VERIFY(device.isExtensionEnabled<Extensions::EXT::debug_marker>());
    CORRADE_VERIFY(device.isExtensionEnabled(Extensions::KHR::maintenance1{}));
    /* ... and function pointers loaded */
    CORRADE_VERIFY(device->CmdDebugMarkerInsertEXT);
    CORRADE_VERIFY(device->TrimCommandPoolKHR);
}

void DeviceVkTest::constructCommandLineDisable() {
    auto&& data = ConstructCommandLineData[testCaseInstanceId()];
    setTestCaseDescription(data.nameDisable);

    if(std::getenv("MAGNUM_VULKAN_VERSION"))
        CORRADE_SKIP("Can't test with the MAGNUM_VULKAN_VERSION environment variable set");
    if(std::getenv("MAGNUM_DISABLE_LAYERS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_LAYERS environment variable set");
    if(std::getenv("MAGNUM_DISABLE_EXTENSIONS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_EXTENSIONS environment variable set");

    if(!enumerateLayerProperties().isSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");

    /* Creating a dedicated instance so we can pass custom args and enable
       layers independently */
    Instance instance{InstanceCreateInfo{Int(data.argsDisable.size()), data.argsDisable}
        .addEnabledLayers({"VK_LAYER_KHRONOS_validation"})
        /* Needed by VK_EXT_debug_marker */
        .addEnabledExtensions<Extensions::EXT::debug_report>()
    };

    DeviceProperties deviceProperties = pickDevice(instance);
    ExtensionProperties extensions = deviceProperties.enumerateExtensionProperties({"VK_LAYER_KHRONOS_validation"});
    if(!extensions.isSupported<Extensions::EXT::debug_marker>())
        CORRADE_SKIP("VK_EXT_debug_marker not supported, can't test");
    if(!extensions.isSupported<Extensions::KHR::maintenance1>())
        CORRADE_SKIP("VK_KHR_maintenance1 not supported, can't test");

    std::ostringstream out;
    Debug redirectOutput{&out};
    Device device{instance, DeviceCreateInfo{deviceProperties, DeviceCreateInfo::Flag::NoImplicitExtensions}
        .addQueues(0, {0.0f})
        .addEnabledExtensions<
            Extensions::EXT::debug_marker,
            Extensions::KHR::maintenance1
        >()};
    CORRADE_VERIFY(device.handle());
    CORRADE_COMPARE(device.isVersionSupported(deviceProperties.apiVersion()), data.driverVersionSupported);
    CORRADE_COMPARE(device.isExtensionEnabled<Extensions::EXT::debug_marker>(), data.debugMarkerEnabled);
    CORRADE_COMPARE(device.isExtensionEnabled<Extensions::KHR::maintenance1>(), data.maintenance1Enabled);

    /** @todo cleanup when Debug::toString() or some similar utility exists */
    UnsignedInt major = versionMajor(deviceProperties.apiVersion());
    UnsignedInt minor = versionMinor(deviceProperties.apiVersion());
    UnsignedInt patch = versionPatch(deviceProperties.apiVersion());
    CORRADE_COMPARE(out.str(), Utility::formatString(data.log, major, minor, patch));

    /* Verify that the entrypoint is actually (not) loaded as expected, to
       avoid all the above reporting being just smoke & mirrors */
    CORRADE_COMPARE(!!device->CmdDebugMarkerInsertEXT, data.debugMarkerEnabled);
    CORRADE_COMPARE(!!device->TrimCommandPoolKHR, data.maintenance1Enabled);
}

void DeviceVkTest::constructCommandLineEnable() {
    auto&& data = ConstructCommandLineData[testCaseInstanceId()];
    setTestCaseDescription(data.nameEnable);

    if(std::getenv("MAGNUM_VULKAN_VERSION"))
        CORRADE_SKIP("Can't test with the MAGNUM_VULKAN_VERSION environment variable set");
    if(std::getenv("MAGNUM_DISABLE_LAYERS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_LAYERS environment variable set");
    if(std::getenv("MAGNUM_DISABLE_EXTENSIONS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_EXTENSIONS environment variable set");

    if(!enumerateLayerProperties().isSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");

    /* Creating a dedicated instance so we can pass custom args and enable
       layers independently */
    Instance instance{InstanceCreateInfo{Int(data.argsEnable.size()), data.argsEnable}
        .addEnabledLayers({"VK_LAYER_KHRONOS_validation"})
        /* Needed by VK_EXT_debug_marker */
        .addEnabledExtensions<Extensions::EXT::debug_report>()
    };

    DeviceProperties deviceProperties = pickDevice(instance);
    ExtensionProperties extensions = deviceProperties.enumerateExtensionProperties({"VK_LAYER_KHRONOS_validation"});
    if(!extensions.isSupported<Extensions::EXT::debug_marker>())
        CORRADE_SKIP("VK_EXT_debug_marker not supported, can't test");
    if(!extensions.isSupported<Extensions::KHR::maintenance1>())
        CORRADE_SKIP("VK_KHR_maintenance1 not supported, can't test");

    std::ostringstream out;
    Debug redirectOutput{&out};
    Device device{instance, DeviceCreateInfo{instance, DeviceCreateInfo::Flag::NoImplicitExtensions}
        .addQueues(0, {0.0f})
        /* Nothing enabled by the application */
    };
    CORRADE_VERIFY(device.handle());
    CORRADE_COMPARE(device.isVersionSupported(deviceProperties.apiVersion()), data.driverVersionSupported);
    CORRADE_COMPARE(device.isExtensionEnabled<Extensions::EXT::debug_marker>(), data.debugMarkerEnabled);
    CORRADE_COMPARE(device.isExtensionEnabled<Extensions::KHR::maintenance1>(), data.maintenance1Enabled);

    /** @todo cleanup when Debug::toString() or some similar utility exists */
    UnsignedInt major = versionMajor(deviceProperties.apiVersion());
    UnsignedInt minor = versionMinor(deviceProperties.apiVersion());
    UnsignedInt patch = versionPatch(deviceProperties.apiVersion());
    CORRADE_COMPARE(out.str(), Utility::formatString(data.log, major, minor, patch));

    /* Verify that the entrypoint is actually (not) loaded as expected, to
       avoid all the above reporting being just smoke & mirrors */
    CORRADE_COMPARE(!!device->CmdDebugMarkerInsertEXT, data.debugMarkerEnabled);
    CORRADE_COMPARE(!!device->TrimCommandPoolKHR, data.maintenance1Enabled);
}

void DeviceVkTest::constructMove() {
    Device a{_instance, DeviceCreateInfo{_instance}
        .addQueues(0, {0.0f})
    };
    CORRADE_VERIFY(a.handle());
    CORRADE_COMPARE(a.handleFlags(), HandleFlag::DestroyOnDestruction);
    CORRADE_VERIFY(a->CreateBuffer);

    Device b = std::move(a);
    CORRADE_VERIFY(!a.handle());
    CORRADE_COMPARE(b.handleFlags(), HandleFlag::DestroyOnDestruction);
    CORRADE_VERIFY(b.handle());
    /* Function pointers in a are left in whatever state they were before, as
       that doesn't matter */
    CORRADE_VERIFY(b->CreateBuffer);

    Device c{NoCreate};
    c = std::move(b);
    CORRADE_VERIFY(!b.handle());
    CORRADE_COMPARE(b.handleFlags(), HandleFlags{});
    CORRADE_COMPARE(c.handleFlags(), HandleFlag::DestroyOnDestruction);
    CORRADE_VERIFY(c.handle());
    /* Everything is swapped, including function pointers */
    CORRADE_VERIFY(!b->CreateBuffer);
    CORRADE_VERIFY(c->CreateBuffer);
}

void DeviceVkTest::constructUnknownExtension() {
    CORRADE_SKIP("Currently this hits an internal assert, which can't be tested.");

    std::ostringstream out;
    Error redirectError{&out};
    Device device{_instance, DeviceCreateInfo{_instance}
        .addQueues(0, {0.0f})
        .addEnabledExtensions({"VK_this_doesnt_exist"_s})};
    CORRADE_COMPARE(out.str(), "TODO");
}

void DeviceVkTest::constructNoQueue() {
    std::ostringstream out;
    Error redirectError{&out};
    Device device{_instance, DeviceCreateInfo{_instance}};
    CORRADE_COMPARE(out.str(), "Vk::Device: needs to be created with at least one queue\n");
}

void DeviceVkTest::wrap() {
    if(std::getenv("MAGNUM_VULKAN_VERSION"))
        CORRADE_SKIP("Can't test with the MAGNUM_VULKAN_VERSION environment variable set");
    if(std::getenv("MAGNUM_DISABLE_LAYERS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_LAYERS environment variable set");
    if(std::getenv("MAGNUM_DISABLE_EXTENSIONS"))
        CORRADE_SKIP("Can't test with the MAGNUM_DISABLE_EXTENSIONS environment variable set");

    if(!enumerateLayerProperties().isSupported("VK_LAYER_KHRONOS_validation"))
        CORRADE_SKIP("VK_LAYER_KHRONOS_validation not supported, can't test");

    /* Creating a dedicated instance so we can enable layers independently */
    Instance instance{InstanceCreateInfo{}
        .addEnabledLayers({"VK_LAYER_KHRONOS_validation"})
        /* Needed by VK_EXT_debug_marker */
        .addEnabledExtensions<Extensions::EXT::debug_report>()
    };

    DeviceProperties deviceProperties = pickDevice(instance);
    ExtensionProperties extensions = deviceProperties.enumerateExtensionProperties({"VK_LAYER_KHRONOS_validation"});
    if(!extensions.isSupported<Extensions::EXT::debug_marker>())
        CORRADE_SKIP("VK_EXT_debug_marker not supported, can't test");
    if(!extensions.isSupported<Extensions::KHR::maintenance1>())
        CORRADE_SKIP("VK_KHR_maintenance1 not supported, can't test");

    VkDevice device;
    CORRADE_COMPARE(Result(instance->CreateDevice(deviceProperties,
        DeviceCreateInfo{instance}
            .addQueues(0, {0.0f})
            .addEnabledExtensions<
                Extensions::EXT::debug_marker,
                Extensions::KHR::maintenance1
            >(),
        nullptr, &device)), Result::Success);
    CORRADE_VERIFY(device);

    {
        /* Wrapping should load the basic function pointers */
        auto wrapped = Device::wrap(instance, device, Version::Vk11, {
            Extensions::EXT::debug_marker::string()
        }, HandleFlag::DestroyOnDestruction);
        CORRADE_VERIFY(wrapped->DestroyDevice);

        /* Specified version should be reported as supported but higher not
           regardless of the actual driver version */
        CORRADE_VERIFY(wrapped.isVersionSupported(Version::Vk11));
        CORRADE_VERIFY(!wrapped.isVersionSupported(Version::Vk12));

        /* Listed extensions should be reported as enabled and function
           pointers loaded as well */
        CORRADE_VERIFY(wrapped.isExtensionEnabled<Extensions::EXT::debug_marker>());
        CORRADE_VERIFY(wrapped->CmdDebugMarkerInsertEXT);

        /* Unlisted not, but function pointers should still be loaded as the
           actual instance does have the extension enabled */
        CORRADE_VERIFY(!wrapped.isExtensionEnabled<Extensions::KHR::maintenance1>());
        CORRADE_VERIFY(wrapped->TrimCommandPoolKHR);

        /* Releasing won't destroy anything ... */
        CORRADE_COMPARE(wrapped.release(), device);
    }

    /* ...so we can wrap it again, non-owned, and then destroy it manually */
    auto wrapped = Device::wrap(instance, device, Version::Vk10, {});
    CORRADE_VERIFY(wrapped->DestroyDevice);
    wrapped->DestroyDevice(device, nullptr);
}

void DeviceVkTest::populateGlobalFunctionPointers() {
    vkDestroyDevice = nullptr;

    Device device{_instance, DeviceCreateInfo{_instance}
        .addQueues(0, {0.0f})
    };
    CORRADE_VERIFY(!vkDestroyDevice);
    device.populateGlobalFunctionPointers();
    CORRADE_VERIFY(vkDestroyDevice);
    CORRADE_VERIFY(vkDestroyDevice == device->DestroyDevice);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Vk::Test::DeviceVkTest)
