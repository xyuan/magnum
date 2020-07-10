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

#include <Corrade/Utility/Arguments.h>

#include "Magnum/Vk/Extensions.h"
#include "Magnum/Vk/ExtensionProperties.h"
#include "Magnum/Vk/Instance.h"
#include "Magnum/Vk/LayerProperties.h"
#include "Magnum/Vk/DeviceProperties.h"
#include "Magnum/Vk/Version.h"

namespace Magnum {

/** @page magnum-vk-info Magnum Vulkan Info
@brief Displays information about Magnum engine Vulkan capabilities

@m_footernavigation
@m_keywords{magnum-vk-info vk-info}

This utility is built if both `WITH_VK` and `WITH_VK_INFO` is enabled when
building Magnum. To use this utility with CMake, you need to request the
`vk-info` component of the `Magnum` package and use the `Magnum::vk-info`
target for example in a custom command:

@code{.cmake}
find_package(Magnum REQUIRED vk-info)

add_custom_command(OUTPUT ... COMMAND Magnum::vk-info ...)
@endcode

See @ref building, @ref cmake and the @ref Vk namespace for more information.
*/

}

using namespace Magnum;

int main(int argc, char** argv) {
    Utility::Arguments args;
    args.addBooleanOption("extension-strings").setHelp("extension-strings", "list all extension strings provided by the driver")
        .addBooleanOption("all-extensions").setHelp("all-extensions", "display extensions also for fully supported versions")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Displays information about Magnum engine and Vulkan capabilities.")
        .parse(argc, argv);

    /* Setup InstanceCreateInfo before printing anything so --magnum-help has
       uncluttered output */
    /** @todo add a NoCreate InstanceInfo that just populates internal state
        without querying vulkan for anything? or that's stupid? */
    Vk::LayerProperties layerProperties = Vk::enumerateLayerProperties();
    Vk::InstanceExtensionProperties instanceExtensionProperties = Vk::enumerateInstanceExtensionProperties({layerProperties.names()});

    Vk::InstanceCreateInfo instanceCreateInfo{argc, argv, &layerProperties, &instanceExtensionProperties};

    Debug{} << "";
    Debug{} << "  +---------------------------------------------------------+";
    Debug{} << "  |   Information about Magnum engine Vulkan capabilities   |";
    Debug{} << "  +---------------------------------------------------------+";
    Debug{} << "";

    Debug{} << "Compilation flags:";
    #ifdef CORRADE_BUILD_DEPRECATED
    Debug{} << "    CORRADE_BUILD_DEPRECATED";
    #endif
    #ifdef CORRADE_BUILD_STATIC
    Debug{} << "    CORRADE_BUILD_STATIC";
    #endif
    #ifdef CORRADE_BUILD_MULTITHREADED
    Debug{} << "    CORRADE_BUILD_MULTITHREADED";
    #endif
    #ifdef CORRADE_TARGET_UNIX
    Debug{} << "    CORRADE_TARGET_UNIX";
    #endif
    #ifdef CORRADE_TARGET_APPLE
    Debug{} << "    CORRADE_TARGET_APPLE";
    #endif
    #ifdef CORRADE_TARGET_IOS
    Debug{} << "    CORRADE_TARGET_IOS";
    #endif
    #ifdef CORRADE_TARGET_WINDOWS
    Debug{} << "    CORRADE_TARGET_WINDOWS";
    #endif
    #ifdef CORRADE_TARGET_WINDOWS_RT
    Debug{} << "    CORRADE_TARGET_WINDOWS_RT";
    #endif
    #ifdef CORRADE_TARGET_ANDROID
    Debug{} << "    CORRADE_TARGET_ANDROID";
    #endif
    #ifdef CORRADE_TARGET_X86
    Debug{} << "    CORRADE_TARGET_X86";
    #endif
    #ifdef CORRADE_TARGET_ARM
    Debug{} << "    CORRADE_TARGET_ARM";
    #endif
    #ifdef CORRADE_TARGET_POWERPC
    Debug{} << "    CORRADE_TARGET_POWERPC";
    #endif
    #ifdef CORRADE_TARGET_BIG_ENDIAN
    Debug{} << "    CORRADE_TARGET_BIG_ENDIAN";
    #endif
    #ifdef CORRADE_TARGET_GCC
    Debug{} << "    CORRADE_TARGET_GCC";
    #endif
    #ifdef CORRADE_TARGET_CLANG
    Debug{} << "    CORRADE_TARGET_CLANG";
    #endif
    #ifdef CORRADE_TARGET_APPLE_CLANG
    Debug{} << "    CORRADE_TARGET_APPLE_CLANG";
    #endif
    #ifdef CORRADE_TARGET_CLANG_CL
    Debug{} << "    CORRADE_TARGET_CLANG_CL";
    #endif
    #ifdef CORRADE_TARGET_MSVC
    Debug{} << "    CORRADE_TARGET_MSVC";
    #endif
    #ifdef CORRADE_TARGET_MINGW
    Debug{} << "    CORRADE_TARGET_MINGW";
    #endif
    #ifdef CORRADE_TARGET_LIBCXX
    Debug{} << "    CORRADE_TARGET_LIBCXX";
    #endif
    #ifdef CORRADE_TARGET_LIBSTDCXX
    Debug{} << "    CORRADE_TARGET_LIBSTDCXX";
    #endif
    #ifdef CORRADE_TARGET_DINKUMWARE
    Debug{} << "    CORRADE_TARGET_DINKUMWARE";
    #endif
    #ifdef CORRADE_TARGET_SSE2
    Debug{} << "    CORRADE_TARGET_SSE2";
    #endif
    #ifdef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    Debug{} << "    CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT";
    #endif
    #ifdef CORRADE_TESTSUITE_TARGET_XCTEST
    Debug{} << "    CORRADE_TESTSUITE_TARGET_XCTEST";
    #endif
    #ifdef CORRADE_UTILITY_USE_ANSI_COLORS
    Debug{} << "    CORRADE_UTILITY_USE_ANSI_COLORS";
    #endif
    #ifdef MAGNUM_BUILD_DEPRECATED
    Debug{} << "    MAGNUM_BUILD_DEPRECATED";
    #endif
    #ifdef MAGNUM_BUILD_STATIC
    Debug{} << "    MAGNUM_BUILD_STATIC";
    #endif
    Debug{} << "";

    const Vk::Version version = Vk::enumerateInstanceVersion();
    Debug{} << "Reported instance version:" << version;
    Debug{} << "Reported instance layers:";
    for(UnsignedInt i = 0, max = layerProperties.count(); i != max; ++i) {
        Debug{} << "   " << layerProperties.name(i) << "(r" << Debug::nospace << layerProperties.revision(i) << Debug::nospace << ", written against" << layerProperties.version(i) << Debug::nospace << ")";
        Debug{} << "     " << layerProperties.description(i);
    }

    constexpr Vk::Version versions[]{
        Vk::Version::Vk11,
        Vk::Version::Vk12,
        Vk::Version::None
    };
    std::size_t future = 0;

    if(!args.isSet("all-extensions"))
        while(versions[future] != Vk::Version::None && version >= versions[future])
            ++future;

    /** @todo do better once implemented in format() */
    using namespace Containers::Literals;
    constexpr Containers::StringView sixtyfourSpaces = "                                                               "_s;

    if(args.isSet("extension-strings")) {
        Debug{} << "Reported instance extension strings:";
        for(std::size_t i = 0, max = instanceExtensionProperties.count(); i != max; ++i) {
            Debug d;
            d << "   " << instanceExtensionProperties.name(i) << "(r" << Debug::nospace << instanceExtensionProperties.revision(i) << Debug::nospace;
            const UnsignedInt layer = instanceExtensionProperties.layer(i);
            if(layer != 0)
                d << ", from" << layerProperties.names()[layer - 1] << Debug::nospace;
            d << ")";
        }

    } else for(std::size_t i = future; i != Containers::arraySize(versions); ++i) {
        Containers::ArrayView<const Vk::InstanceExtension> extensions = Vk::InstanceExtension::extensions(versions[i]);
        if(extensions.empty()) continue;

        if(versions[i] != Vk::Version::None)
            Debug{} << versions[i] << "instance extension support:";
        else Debug{} << "Vendor instance extension support:";

        for(const Vk::InstanceExtension& extension: extensions) {
            Debug d;
            d << "   " << extension.string() << sixtyfourSpaces.prefix(64 - extension.string().size());

            if(instanceExtensionProperties.isSupported(extension))
                d << "REV." << Debug::nospace << instanceExtensionProperties.revision(extension);
            else if(version >= extension.requiredVersion())
                d << "  -";
            else
                d << " n/a";
        }
    }

    Debug{} << "";

    Vk::Instance instance{instanceCreateInfo};

    {
        Containers::Array<Vk::DeviceProperties> devices = Vk::enumerateDevices(instance);
        Debug{} << "Found" << devices.size() << "devices:";
        for(Vk::DeviceProperties& device: devices) {
            Debug{} << "   " << device.name() << Debug::nospace << ","
                << device.apiVersion() << Debug::newline
                << "     " << device.type() << Debug::nospace << ", driver"
                << Debug::packed << device.driverVersion();
        }

        if(devices.empty()) return 0;
    }

    Debug{} << "";

    Vk::DeviceProperties device = Vk::pickDevice(instance);

    Debug{} << "Picked device" << device.name() << Debug::newline;
    Debug{} << "Reported version:" << device.apiVersion();

    Vk::ExtensionProperties extensionProperties = device.enumerateExtensionProperties(layerProperties.names());

    if(args.isSet("extension-strings")) {
        Debug{} << "Reported extension strings:";
        for(std::size_t i = 0, max = extensionProperties.count(); i != max; ++i) {
            Debug d;
            d << "   " << extensionProperties.name(i) << "(r" << Debug::nospace << extensionProperties.revision(i) << Debug::nospace;
            const UnsignedInt layer = extensionProperties.layer(i);
            if(layer != 0)
                d << ", from" << layerProperties.names()[layer - 1] << Debug::nospace;
            d << ")";
        }

    } else for(std::size_t i = future; i != Containers::arraySize(versions); ++i) {
        Containers::ArrayView<const Vk::Extension> extensions = Vk::Extension::extensions(versions[i]);
        if(extensions.empty()) continue;

        if(versions[i] != Vk::Version::None)
            Debug{} << versions[i] << "extension support:";
        else Debug{} << "Vendor extension support:";

        for(const Vk::Extension& extension: extensions) {
            Debug d;
            d << "   " << extension.string() << sixtyfourSpaces.prefix(64 - extension.string().size());

            if(extensionProperties.isSupported(extension))
                d << "REV." << Debug::nospace << extensionProperties.revision(extension);
            else if(version >= extension.requiredVersion())
                d << "  -";
            else
                d << " n/a";
        }
    }

    /* If we wanted only extension strings, exit now */
    if(args.isSet("extension-strings")) return 0;

    Debug{} << "Queue families:";
    for(UnsignedInt i = 0; i != device.queueFamilyCount(); ++i) {
        Debug{} << "   " << i << Debug::nospace << ":" << device.queueFamilyFlags(i);
        Debug{} << "     " << device.queueFamilySize(i) << "queues";
    }
}
