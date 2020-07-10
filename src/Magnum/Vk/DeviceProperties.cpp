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

#include "DeviceProperties.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Debug.h>

#include "Magnum/Vk/Instance.h"
#include "Magnum/Vk/ExtensionProperties.h"
#include "Magnum/Vk/LayerProperties.h"
#include "Magnum/Vk/Result.h"
#include "Magnum/Vk/Implementation/Arguments.h"
#include "Magnum/Vk/Implementation/InstanceState.h"

namespace Magnum { namespace Vk {

struct DeviceProperties::State {
    VkPhysicalDeviceProperties2 properties{};
    Containers::Array<VkQueueFamilyProperties2> queueFamilyProperties;
};

DeviceProperties::DeviceProperties(Instance& instance, VkPhysicalDevice handle): _instance{instance}, _handle{handle} {}

/* The VkDeviceProperties handle doesn't need to be destroyed so it's enough to
   just rely on the implicit behavior */
DeviceProperties::DeviceProperties(DeviceProperties&&) noexcept = default;

DeviceProperties::~DeviceProperties() = default;

DeviceProperties& DeviceProperties::operator=(DeviceProperties&&) noexcept = default;

Containers::StringView DeviceProperties::name() {
    return properties().properties.deviceName;
}

const VkPhysicalDeviceProperties2& DeviceProperties::properties() {
    if(!_state) _state.emplace();

    /* Properties not fetched yet, do that now */
    if(!_state->properties.sType) {
        _state->properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        _instance->state().getPhysicalDevicePropertiesImplementation(*this, _state->properties);
    }

    return _state->properties;
}

void DeviceProperties::getPropertiesImplementationDefault(DeviceProperties& self, VkPhysicalDeviceProperties2& properties) {
    return (**self._instance).GetPhysicalDeviceProperties(self._handle, &properties.properties);
}

void DeviceProperties::getPropertiesImplementationKHR(DeviceProperties& self, VkPhysicalDeviceProperties2& properties) {
    return (**self._instance).GetPhysicalDeviceProperties2KHR(self._handle, &properties);
}

void DeviceProperties::getPropertiesImplementationVulkan11(DeviceProperties& self, VkPhysicalDeviceProperties2& properties) {
    return (**self._instance).GetPhysicalDeviceProperties2(self._handle, &properties);
}

ExtensionProperties DeviceProperties::enumerateExtensionProperties(Containers::ArrayView<const Containers::StringView> layers) {
    return InstanceExtensionProperties{layers, [](void* state, const char* const layer, UnsignedInt* count, VkExtensionProperties* properties) {
        auto& deviceProperties = *static_cast<DeviceProperties*>(state);
        return (**deviceProperties._instance).EnumerateDeviceExtensionProperties(deviceProperties._handle, layer, count, properties);
    }, this};
}

ExtensionProperties DeviceProperties::enumerateExtensionProperties(std::initializer_list<Containers::StringView> layers) {
    return enumerateExtensionProperties(Containers::arrayView(layers));
}

Containers::ArrayView<const VkQueueFamilyProperties2> DeviceProperties::queueFamilyProperties() {
    if(!_state) _state.emplace();

    /* Fetch if not already */
    if(_state->queueFamilyProperties.empty()) {
        UnsignedInt count;
        _instance->state().getPhysicalDeviceQueueFamilyProperties(*this, count, nullptr);

        _state->queueFamilyProperties = Containers::Array<VkQueueFamilyProperties2>{Containers::NoInit, count};
        _instance->state().getPhysicalDeviceQueueFamilyProperties(*this, count, _state->queueFamilyProperties);
        CORRADE_INTERNAL_ASSERT(count == _state->queueFamilyProperties.size());
    }

    return _state->queueFamilyProperties;
}

void DeviceProperties::getQueueFamilyPropertiesImplementationDefault(DeviceProperties& self, UnsignedInt& count, VkQueueFamilyProperties2* properties) {
    (**self._instance).GetPhysicalDeviceQueueFamilyProperties(self._handle,  &count, reinterpret_cast<VkQueueFamilyProperties*>(properties));

    /* "Sparsen" the returned data to the version 2 structure layout. Go
       backwards so we don't overwrite the yet-to-be-processed data; if the
       pointer is null we were just querying the count */
    if(properties) {
        Containers::ArrayView<VkQueueFamilyProperties> src{reinterpret_cast<VkQueueFamilyProperties*>(properties), count};
        Containers::ArrayView<VkQueueFamilyProperties2> dst{properties, count};
        for(std::size_t i = count; i != 0; --i) {
            dst[i - 1].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
            dst[i - 1].pNext = nullptr;
            dst[i - 1].queueFamilyProperties = src[i - 1];
        }
    }
}

void DeviceProperties::getQueueFamilyPropertiesImplementationKHR(DeviceProperties& self, UnsignedInt& count, VkQueueFamilyProperties2* properties) {
    return (**self._instance).GetPhysicalDeviceQueueFamilyProperties2KHR(self._handle, &count, properties);
}

void DeviceProperties::getQueueFamilyPropertiesImplementationVulkan11(DeviceProperties& self, UnsignedInt& count, VkQueueFamilyProperties2* properties) {
    return (**self._instance).GetPhysicalDeviceQueueFamilyProperties2(self._handle, &count, properties);
}

UnsignedInt DeviceProperties::queueFamilyCount() {
    return queueFamilyProperties().size();
}

UnsignedInt DeviceProperties::queueFamilySize(const UnsignedInt id) {
    const Containers::ArrayView<const VkQueueFamilyProperties2> properties = queueFamilyProperties();
    CORRADE_ASSERT(id < properties.size(),
        "Vk::DeviceProperties::queueFamilySize(): index" << id << "out of range for" << properties.size() << "entries", {});
    return properties[id].queueFamilyProperties.queueCount;
}

QueueFlags DeviceProperties::queueFamilyFlags(const UnsignedInt id) {
    const Containers::ArrayView<const VkQueueFamilyProperties2> properties = queueFamilyProperties();
    CORRADE_ASSERT(id < properties.size(),
        "Vk::DeviceProperties::flags(): index" << id << "out of range for" << properties.size() << "entries", {});
    return QueueFlag(properties[id].queueFamilyProperties.queueFlags);
}

UnsignedInt DeviceProperties::pickQueueFamily(const QueueFlags flags) {
    Containers::Optional<UnsignedInt> id = tryPickQueueFamily(flags);
    if(id) return *id;
    std::exit(1);
}

Containers::Optional<UnsignedInt> DeviceProperties::tryPickQueueFamily(const QueueFlags flags) {
    const Containers::ArrayView<const VkQueueFamilyProperties2> properties = queueFamilyProperties();
    for(UnsignedInt i = 0; i != properties.size(); ++i)
        if(QueueFlag(properties[i].queueFamilyProperties.queueFlags) >= flags) return i;

    Error{} << "Vk::DeviceProperties::tryPickQueueFamily(): no" << flags << "found among" << properties.size() << "queue families";
    return {};
}

Containers::Array<DeviceProperties> enumerateDevices(Instance& instance) {
    /* Retrieve total device count */
    UnsignedInt count;
    MAGNUM_VK_INTERNAL_ASSERT_RESULT(instance->EnumeratePhysicalDevices(instance, &count, nullptr));

    /* Allocate memory for the output, fetch the handles into it */
    Containers::Array<DeviceProperties> out{Containers::NoInit, count};
    Containers::ArrayView<VkPhysicalDevice> handles{reinterpret_cast<VkPhysicalDevice*>(out.data()), count};
    MAGNUM_VK_INTERNAL_ASSERT_RESULT(instance->EnumeratePhysicalDevices(instance, &count, handles.data()));

    /* Expect the device count didn't change between calls */
    CORRADE_INTERNAL_ASSERT(count == out.size());

    /* Construct actual DeviceProperties instances from these, go backwards so
       we don't overwrite the not-yet-processed handles */
    for(std::size_t i = count; i != 0; --i)
        new(out.data() + i - 1) DeviceProperties{instance, handles[i - 1]};

    return out;
}

Containers::Optional<DeviceProperties> tryPickDevice(Instance& instance) {
    Utility::Arguments args = Implementation::arguments();
    args.parse(instance.state().argc, instance.state().argv);

    Containers::Array<DeviceProperties> devices = enumerateDevices(instance);

    /* Pick the first by default */
    if(args.value("device").empty()) {
        if(devices.empty()) {
            Error{} << "Vk::tryPickPhysicalDevice(): no Vulkan devices found";
            return {};
        }
        return std::move(devices.front());
    }

    /* Pick by ID */
    if(args.value("device")[0] >= '0' && args.value("device")[0] <= '9') {
        UnsignedInt id = args.value<UnsignedInt>("device");
        if(id >= devices.size()) {
            Error{} << "Vk::tryPickPhysicalDevice(): index" << id << "out of bounds for" << devices.size() << "Vulkan devices";
            return {};
        }
        return std::move(devices[id]);
    }

    /* Pick by type */
    DeviceType type;
    if(args.value("device") == "integrated")
        type = DeviceType::IntegratedGpu;
    else if(args.value("device") == "discrete")
        type = DeviceType::DiscreteGpu;
    else if(args.value("device") == "virtual")
        type = DeviceType::VirtualGpu;
    else if(args.value("device") == "cpu")
        type = DeviceType::Cpu;
    else {
        Error{} << "Vk::tryPickPhysicalDevice(): unknown Vulkan device type" << args.value<Containers::StringView>("device");
        return {};
    }

    for(DeviceProperties& device: devices)
        if(device.type() == type) return std::move(device);

    Error{} << "Vk::tryPickPhysicalDevice(): no" << type << "found among" << devices.size() << "Vulkan devices";
    return {};
}

DeviceProperties pickDevice(Instance& instance) {
    Containers::Optional<DeviceProperties> device = tryPickDevice(instance);
    if(device) return *std::move(device);
    std::exit(1);
}

Debug& operator<<(Debug& debug, const DeviceType value) {
    debug << "Vk::DeviceType" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Vk::DeviceType::value: return debug << "::" << Debug::nospace << #value;
        _c(Other)
        _c(IntegratedGpu)
        _c(DiscreteGpu)
        _c(VirtualGpu)
        _c(Cpu)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    /* Vulkan docs have the values in decimal, so not converting to hex */
    return debug << "(" << Debug::nospace << Int(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const QueueFlag value) {
    debug << "Vk::QueueFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Vk::QueueFlag::value: return debug << "::" << Debug::nospace << #value;
        _c(Graphics)
        _c(Compute)
        _c(Transfer)
        _c(SparseBinding)
        _c(Protected)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    /* Flag bits should be in hex, unlike plain values */
    return debug << "(" << Debug::nospace << reinterpret_cast<void*>(UnsignedInt(value)) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const QueueFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Vk::QueueFlags{}", {
        Vk::QueueFlag::Graphics,
        Vk::QueueFlag::Compute,
        Vk::QueueFlag::Transfer,
        Vk::QueueFlag::SparseBinding,
        Vk::QueueFlag::Protected});
}

}}
