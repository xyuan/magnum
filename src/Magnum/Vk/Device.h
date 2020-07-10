#ifndef Magnum_Vk_Device_h
#define Magnum_Vk_Device_h
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

/** @file
 * @brief Class @ref Magnum::Vk::DeviceCreateInfo, @ref Magnum::Vk::Device
 * @m_since_latest
 */

#include <cstddef>
#include <Corrade/Containers/Pointer.h>

#include "Magnum/Tags.h"
#include "Magnum/Math/BoolVector.h"
#include "Magnum/Vk/Handle.h"
#include "Magnum/Vk/TypeTraits.h"
#include "Magnum/Vk/Vk.h"
#include "Magnum/Vk/Vulkan.h"
#include "Magnum/Vk/visibility.h"

namespace Magnum { namespace Vk {

namespace Implementation {
    enum: std::size_t { ExtensionCount = 72 };

    struct DeviceState;
}

/**
@brief Device creation info
@m_since_latest

Wraps @type_vk_keyword{DeviceCreateInfo}.
@see @ref Device::Device(const DeviceCreateInfo&)
*/
class MAGNUM_VK_EXPORT DeviceCreateInfo {
    public:
        /**
         * @brief Device flag
         *
         * @see @ref Flags, @ref DeviceCreateInfo(DeviceProperties&, const ExtensionProperties*, Flags)
         */
        enum class Flag: UnsignedInt {
            /* Any magnum-specific flags added here have to be filtered out
               when passing them to _info.flags in the constructor. Using the
               highest bits in a hope to prevent conflicts with Vulkan instance
               flags added in the future. */

            /**
             * Don't implicitly enable any extensions.
             *
             * By default, the engine enables various extensions such as
             * @vk_extension{KHR,get_memory_requirements2} to provide a broader
             * functionality. If you want to have a complete control over what
             * gets enabled, set this flag.
             */
            NoImplicitExtensions = 1u << 31
        };

        /**
         * @brief Device flags
         *
         * @see @ref DeviceCreateInfo(Int, const char**, DeviceProperties*, const ExtensionProperties*, Flags)
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Constructor
         * @param deviceProperties  A device to use
         * @param extensionProperties Existing @ref ExtensionProperties
         *      instance for querying available Vulkan extensions. If
         *      @cpp nullptr @ce and @p deviceProperties is set, a new instance
         *      may be created internally if needed.
         * @param flags             Device flags
         *
         * The following values are pre-filled in addition to `sType`:
         *
         * -    *(none)*
         */
        explicit DeviceCreateInfo(DeviceProperties& deviceProperties, const ExtensionProperties* extensionProperties, Flags flags = {});

        /** @overload */
        explicit DeviceCreateInfo(DeviceProperties&& deviceProperties, const ExtensionProperties* extensionProperties, Flags flags = {});

        /** @overload */
        explicit DeviceCreateInfo(DeviceProperties& deviceProperties, Flags flags = {}): DeviceCreateInfo{deviceProperties, nullptr, flags} {}

        /** @overload */
        explicit DeviceCreateInfo(DeviceProperties&& deviceProperties, Flags flags = {}): DeviceCreateInfo{deviceProperties, nullptr, flags} {}

        /**
         * @brief Construct for an implicitly picked device
         *
         * Calls @ref DeviceCreateInfo(DeviceProperties&, const ExtensionProperties*, Flags)
         * with a device picked from @p instance using @ref pickDevice().
         */
        explicit DeviceCreateInfo(Instance& instance, Flags flags = {});

        /**
         * @brief Construct without initializing the contents
         *
         * Note that not even the `sType` field is set --- the structure has to
         * be fully initialized afterwards in order to be usable.
         */
        explicit DeviceCreateInfo(NoInitT) noexcept;

        /**
         * @brief Construct from existing data
         *
         * Copies the existing values verbatim, pointers are kept unchanged
         * without taking over the ownership. Modifying the newly created
         * instance will not modify the original data or the pointed-to data.
         */
        explicit DeviceCreateInfo(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& info) noexcept;

        ~DeviceCreateInfo();

        /**
         * @brief Add enabled device extensions
         * @return Reference to self (for method chaining)
         *
         * All listed extensions are expected to be supported either globally
         * or in at least one of the enabled layers, use
         * @ref ExtensionProperties::isSupported() to check for their presence.
         *
         * The function makes copies of string views that are not owning or
         * null-terminated, use the @link Containers::Literals::operator""_s() @endlink
         * literal to prevent that where possible.
         */
        DeviceCreateInfo& addEnabledExtensions(Containers::ArrayView<const Containers::StringView> extensions);
        /** @overload */
        DeviceCreateInfo& addEnabledExtensions(std::initializer_list<Containers::StringView> extension);
        /** @overload */
        DeviceCreateInfo& addEnabledExtensions(Containers::ArrayView<const Extension> extensions);
        /** @overload */
        DeviceCreateInfo& addEnabledExtensions(std::initializer_list<Extension> extension);
        /** @overload */
        template<class ...E> DeviceCreateInfo& addEnabledExtensions() {
            static_assert(Implementation::IsExtension<E...>::value, "expected only Vulkan device extensions");
            return addEnabledExtensions({E{}...});
        }

        /**
         * @brief Add queues
         * @param family        Family index from @ref QueueFamilyProperties
         * @param priorities    Queue priorities. Size of the array implies how
         *      many queues to add and has to be at least one.
         * @return Reference to self (for method chaining)
         *
         * By default there's one graphics + compute queue added, the first
         * call to @ref addQueues replaces it.
         * @see @ref QueueFamilyProperties::pickFamily()
         */
        DeviceCreateInfo& addQueues(UnsignedInt family, Containers::ArrayView<const Float> priorities);
        /** @overload */
        DeviceCreateInfo& addQueues(UnsignedInt family, std::initializer_list<Float> priorities);

        /**
         * @brief Add queues using raw info
         * @return Reference to self (for method chaining)
         *
         * Compared to @ref addQueues() this allows you to specify additional
         * queue properties using the `pNext` chain. The info is uses as-is,
         * with all pointers expected to stay in scope until device creation.
         */
        DeviceCreateInfo& addQueues(const VkDeviceQueueCreateInfo& info);

        /** @brief Underlying @type_vk{DeviceCreateInfo} structure */
        VkDeviceCreateInfo& operator*() { return _info; }
        /** @overload */
        const VkDeviceCreateInfo& operator*() const { return _info; }
        /** @overload */
        VkDeviceCreateInfo* operator->() { return &_info; }
        /** @overload */
        const VkDeviceCreateInfo* operator->() const { return &_info; }
        /** @overload */
        operator const VkDeviceCreateInfo*() const { return &_info; }

    private:
        friend Device;

        VkPhysicalDevice _physicalDevice;
        VkDeviceCreateInfo _info;
        struct State;
        Containers::Pointer<State> _state;
};

CORRADE_ENUMSET_OPERATORS(DeviceCreateInfo::Flags)

/**
@brief Device
@m_since_latest

Wraps a @type_vk_keyword{Device} and stores all device-specific function
pointers.
*/
class MAGNUM_VK_EXPORT Device {
    public:
        /**
         * @brief Wrap existing Vulkan instance
         * @param instance      Vulkan instance
         * @param handle        The @type_vk{Device} handle
         * @param version       Vulkan version that's assumed to be used on the
         *      instance
         * @param enabledExtensions Extensions that are assumed to be enabled
         *      on the instance
         * @param flags         Handle flags
         *
         * The @p handle is expected to be originating from @p instance. The
         * @p version and @p enabledExtensions parameters populate internal
         * info about supported version and extensions and will be reflected in
         * @ref isVersionSupported() and @ref isExtensionEnabled(), among other
         * things. If @p enabledExtensions is empty, the device will behave as
         * if no extensions were enabled.
         *
         * Unlike a device created using a constructor, the Vulkan device is by
         * default not deleted on destruction, use @p flags for different
         * behavior.
         * @see @ref release()
         */
        static Device wrap(Instance& instance, VkDevice handle, Version version, Containers::ArrayView<const Containers::StringView> enabledExtensions, HandleFlags flags = {});

        /** @overload */
        static Device wrap(Instance& instance, VkDevice handle, Version version, std::initializer_list<Containers::StringView> enabledExtensions, HandleFlags flags = {});

        /**
         * @brief Default constructor
         *
         * @see @fn_vk_keyword{CreateDevice}
         */
        explicit Device(Instance& instance, const DeviceCreateInfo& info);

        /**
         * @brief Construct without creating the instance
         *
         * The constructed instance is equivalent to moved-from state. Useful
         * in cases where you will overwrite the instance later anyway. Move
         * another object over it to make it useful.
         */
        explicit Device(NoCreateT);

        /** @brief Copying is not allowed */
        Device(const Device&) = delete;

        /** @brief Move constructor */
        Device(Device&& other) noexcept;

        /**
         * @brief Destructor
         *
         * Destroys associated @type_vk{Device} object, unless the instance
         * was created using @ref wrap() without @ref HandleFlag::DestroyOnDestruction
         * specified.
         * @see @fn_vk_keyword{DestroyDevice}, @ref release()
         */
        ~Device();

        /** @brief Copying is not allowed */
        Device& operator=(const Device&) = delete;

        /** @brief Move assignment */
        Device& operator=(Device&& other) noexcept;

        /** @brief Underlying @type_vk{Device} handle */
        VkDevice handle() { return _handle; }
        /** @overload */
        operator VkDevice() { return _handle; }

        /** @brief Handle flags */
        HandleFlags handleFlags() const { return _flags; }

        /** @brief Device version */
        Version version() const { return _version; }

        /** @brief Whether given version is supported on the device */
        bool isVersionSupported(Version version) const {
            return _version >= version;
        }

        /**
         * @brief Whether given extension is enabled
         *
         * Accepts device extensions from the @ref Extensions namespace, listed
         * also in the @ref vulkan-support "Vulkan support tables". Search
         * complexity is @f$ \mathcal{O}(1) @f$. Example usage:
         *
         * @snippet MagnumVk.cpp Device-isExtensionEnabled
         *
         * Note that this returns @cpp true @ce only if given extension is
         * supported by the driver *and* it was enabled in
         * @ref DeviceCreateInfo when creating the @ref Device. For querying
         * extension support before creating a device use
         * @ref ExtensionProperties::isSupported().
         */
        template<class E> bool isExtensionEnabled() const {
            static_assert(Implementation::IsExtension<E>::value, "expected a Vulkan device extension");
            return _extensionStatus[E::Index];
        }

        /** @overload */
        bool isExtensionEnabled(const Extension& extension) const;

        /**
         * @brief Device-specific Vulkan function pointers
         *
         * Function pointers are implicitly stored per-device, use
         * @ref populateGlobalFunctionPointers() to populate the global `vk*`
         * functions.
         */
        const FlextVkDevice& operator*() const { return _functionPointers; }
        /** @overload */
        const FlextVkDevice* operator->() const { return &_functionPointers; }

        /**
         * @brief Release the underlying Vulkan devic
         *
         * Releases ownership of the Vulkan device and returns its handle so
         * @fn_vk{DestroyDevice} is not called on destruction. The internal
         * state is then equivalent to moved-from state.
         * @see @ref wrap()
         */
        VkDevice release();

        /**
         * @brief Populate global device-level function pointers to be used with third-party code
         *
         * Populates device-level global function pointers so third-party
         * code is able to call global device-level `vk*` functions:
         *
         * @snippet MagnumVk.cpp Device-global-function-pointers
         *
         * Use @ref Instance::populateGlobalFunctionPointers() to populate
         * instance-level global function pointers.
         * @attention This operation is changing global state. You need to
         *      ensure that this function is not called simultaenously from
         *      multiple threads and code using those function points is
         *      calling them with the same device as the one returned by
         *      @ref handle().
         */
        void populateGlobalFunctionPointers();

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #endif
        Implementation::DeviceState& state() { return *_state; }

    private:
        template<class T> MAGNUM_VK_LOCAL void initializeExtensions(Containers::ArrayView<const T> enabledExtensions);
        MAGNUM_VK_LOCAL void initialize(Instance& instance, Version version);

        VkDevice _handle;
        HandleFlags _flags;
        Version _version;
        Math::BoolVector<Implementation::ExtensionCount> _extensionStatus;
        Containers::Pointer<Implementation::DeviceState> _state;

        /* This member is bigger than you might think */
        FlextVkDevice _functionPointers;
};

}}

#endif
