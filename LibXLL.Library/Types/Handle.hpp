/**
 * @file Handle.hpp
 * @brief Named binary-data handle for persisting user-defined types with an Excel workbook.
 *
 * Provides `xll::Handle<T>`, a typed wrapper around Excel's binary-name mechanism
 * (`xlDefineBinaryName` / `xlGetBinaryName`).  A binary name is a named block of
 * unstructured memory attached to a worksheet that is saved with the workbook.
 *
 * **Serialization**
 *
 * By default T must be trivially copyable; its raw bytes are written directly.
 * For non-trivially-copyable types, specialize `xll::HandleSerializer<T>` and
 * provide `serialize` / `deserialize` overloads.
 *
 * **Limitations inherited from the Excel SDK**
 * - A binary name is associated with the *active* worksheet at creation time.
 * - `store()`, `load()`, `exists()` and `remove()` only work while that sheet
 *   is the active sheet.
 * - These operations must be called from a command or macro-sheet-equivalent
 *   function; they cannot be called from a plain recalculating worksheet function.
 *
 * **Warning on pointer-bearing types**
 *
 * Even if a type is trivially copyable, any embedded raw pointer will be saved
 * as a numeric address and will be stale after a reload.  Store self-contained
 * value types only, or provide a custom `HandleSerializer<T>` that performs
 * proper serialization.
 *
 * @see xlDefineBinaryName, xlGetBinaryName
 * @author Kenneth Balslev
 */

#pragma once

#include "String.hpp"
#include <xlcall.hpp>

#include <cstring>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace xll
{
    // =========================================================================
    // HandleSerializer<T>
    // =========================================================================

    /**
     * @brief Serialization policy for `Handle<T>`.
     *
     * The primary template handles trivially copyable types by copying raw bytes.
     * Specialize this struct for any T that requires custom serialization:
     *
     * @code
     * template<>
     * struct xll::HandleSerializer<MyType> {
     *     static std::vector<std::byte> serialize(const MyType& v) { ... }
     *     static MyType deserialize(const std::byte* data, std::size_t size) { ... }
     * };
     * @endcode
     *
     * @tparam T  The type to be serialized.
     */
    template<typename T>
    struct HandleSerializer
    {
        static_assert(std::is_trivially_copyable_v<T>,
                      "xll::Handle<T>: T must be trivially copyable, "
                      "or a HandleSerializer<T> specialization must be provided.");

        /**
         * @brief Serialize @p value into a byte buffer.
         * @param value  The value to serialize.
         * @return A vector of bytes containing the serialized representation.
         */
        static std::vector<std::byte> serialize(const T& value)
        {
            std::vector<std::byte> buf(sizeof(T));
            std::memcpy(buf.data(), std::addressof(value), sizeof(T));
            return buf;
        }

        /**
         * @brief Deserialize a value from a raw byte buffer.
         * @param data  Pointer to the raw bytes.
         * @param size  Number of bytes available.
         * @return The deserialized value.
         * @throws std::runtime_error if @p size is smaller than `sizeof(T)`.
         */
        static T deserialize(const std::byte* data, std::size_t size)
        {
            if (size < sizeof(T))
                throw std::runtime_error(
                    "xll::Handle: binary data is too small to deserialize T "
                    "(stored data may be from an incompatible version)");
            T value;
            std::memcpy(std::addressof(value), data, sizeof(T));
            return value;
        }
    };

    // =========================================================================
    // Handle<T>
    // =========================================================================

    /**
     * @brief A value of type T that can be stored as a named binary block
     *        associated with the active Excel worksheet.
     *
     * `Handle<T>` owns a single value of type T and exposes operations to
     * persist it to — and retrieve it from — Excel's binary-name storage.
     * The stored data is saved with the workbook and survives workbook close
     * and reopen cycles.
     *
     * Typical usage pattern:
     * @code
     * struct Config { double alpha; int iterations; bool verbose; };
     *
     * // ── Saving ─────────────────────────────────────────────────────────
     * xll::Handle<Config> h({ 0.5, 100, true });
     * h.store("MyConfig");            // persists to the active sheet
     *
     * // ── Modifying and re-saving ─────────────────────────────────────────
     * h->alpha = 0.75;
     * h.store("MyConfig");            // overwrites previous blob
     *
     * // ── Loading ─────────────────────────────────────────────────────────
     * auto loaded = xll::Handle<Config>::load("MyConfig");
     * if (loaded) {
     *     double a = loaded->alpha;   // 0.75
     * }
     *
     * // ── Checking existence ──────────────────────────────────────────────
     * bool ok = xll::Handle<Config>::exists("MyConfig");
     *
     * // ── Deleting ────────────────────────────────────────────────────────
     * xll::Handle<Config>::remove("MyConfig");
     * @endcode
     *
     * @tparam T  The user-defined type to persist.  Must be trivially copyable
     *             by default; supply a `HandleSerializer<T>` specialization for
     *             custom types.
     */
    template<typename T>
    class Handle
    {
        using Serializer = HandleSerializer<T>;

        T data_{};

    public:
        // =====================================================================
        // Construction
        // =====================================================================

        /// Default-constructs the contained value.
        Handle() = default;

        /**
         * @brief Constructs a Handle holding a copy/move of @p value.
         * @param value  Initial value.
         */
        explicit Handle(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : data_(std::move(value))
        {}

        // =====================================================================
        // Value access
        // =====================================================================

        /// Returns a reference to the contained value.
        [[nodiscard]] T& operator*() noexcept { return data_; }
        /// Returns a const reference to the contained value.
        [[nodiscard]] const T& operator*() const noexcept { return data_; }

        /// Provides pointer-like access to the contained value.
        [[nodiscard]] T* operator->() noexcept { return &data_; }
        /// Provides const pointer-like access to the contained value.
        [[nodiscard]] const T* operator->() const noexcept { return &data_; }

        /// Returns a reference to the contained value.
        [[nodiscard]] T& value() noexcept { return data_; }
        /// Returns a const reference to the contained value.
        [[nodiscard]] const T& value() const noexcept { return data_; }

        // =====================================================================
        // Persistence
        // =====================================================================

        /**
         * @brief Serialize and store the contained value as a binary name on
         *        the active worksheet.
         *
         * If a binary block with @p name already exists on the active sheet it
         * is silently replaced.
         *
         * Must be called from a command or macro-sheet-equivalent context.
         *
         * @param name  Case-insensitive identifier for the binary block.
         * @return `true` on success, `false` if the Excel call fails.
         */
        bool store(std::string_view name) const
        {
            const std::vector<std::byte> bytes = Serializer::serialize(data_);

            xll::String xlName(name);

            XLOPER12 xlBig {};
            xlBig.xltype                = xltypeBigData;
            xlBig.val.bigdata.h.lpbData = const_cast<BYTE*>(    // NOLINT(*-const-cast)
                reinterpret_cast<const BYTE*>(bytes.data()));
            xlBig.val.bigdata.cbData = static_cast<long>(bytes.size());

            return Excel12(xlDefineBinaryName, nullptr, 2,
                           static_cast<XLOPER12*>(&xlName), &xlBig) == xlretSuccess;
        }

        /**
         * @brief Load a `Handle<T>` from a binary name on the active worksheet.
         *
         * Retrieves the stored blob via `xlGetBinaryName`, deserializes it using
         * `HandleSerializer<T>`, and returns the result.
         *
         * On Windows, the data is accessed through a `GlobalLock` / `GlobalUnlock`
         * pair as required by the Excel SDK.
         *
         * @param name  Case-insensitive identifier for the binary block.
         * @return A `Handle<T>` containing the loaded value, or `std::nullopt`
         *         if the name does not exist or the operation fails.
         * @throws Whatever `HandleSerializer<T>::deserialize` may throw.
         */
        static std::optional<Handle<T>> load(std::string_view name)
        {
            xll::String xlName(name);

            XLOPER12 xlBig {};
            if (Excel12(xlGetBinaryName, &xlBig, 1,
                        static_cast<XLOPER12*>(&xlName)) != xlretSuccess)
                return std::nullopt;

            if (xlBig.xltype != xltypeBigData)
                return std::nullopt;

            const auto size = static_cast<std::size_t>(xlBig.val.bigdata.cbData);

#ifdef _WIN32
            const auto* raw =
                static_cast<const BYTE*>(::GlobalLock(xlBig.val.bigdata.h.hdata));
            if (!raw)
                return std::nullopt;

            // Deserialize inside a try/catch to guarantee GlobalUnlock is called
            // even if deserialization throws.
            std::optional<Handle<T>> result;
            try {
                result = Handle<T>(Serializer::deserialize(
                    reinterpret_cast<const std::byte*>(raw), size));
            }
            catch (...) {
                ::GlobalUnlock(xlBig.val.bigdata.h.hdata);
                throw;
            }
            ::GlobalUnlock(xlBig.val.bigdata.h.hdata);
            return result;
#else
            // On non-Windows platforms the data pointer is returned directly.
            const auto* raw = xlBig.val.bigdata.h.lpbData;
            if (!raw)
                return std::nullopt;
            return Handle<T>(Serializer::deserialize(
                reinterpret_cast<const std::byte*>(raw), size));
#endif
        }

        /**
         * @brief Check whether a binary name exists on the active worksheet.
         *
         * @param name  Case-insensitive identifier for the binary block.
         * @return `true` if a binary block with @p name exists on the active sheet.
         */
        static bool exists(std::string_view name)
        {
            xll::String xlName(name);
            XLOPER12    xlBig {};
            const int   ret = Excel12(xlGetBinaryName, &xlBig, 1,
                                      static_cast<XLOPER12*>(&xlName));
            return ret == xlretSuccess && xlBig.xltype == xltypeBigData;
        }

        /**
         * @brief Delete a binary name from the active worksheet.
         *
         * Calls `xlDefineBinaryName` with only the name argument and no data,
         * which instructs Excel to remove the named block.
         *
         * Must be called from a command or macro-sheet-equivalent context.
         *
         * @param name  Case-insensitive identifier for the binary block to delete.
         * @return `true` on success, `false` if the Excel call fails.
         */
        static bool remove(std::string_view name)
        {
            xll::String xlName(name);
            return Excel12(xlDefineBinaryName, nullptr, 1,
                           static_cast<XLOPER12*>(&xlName)) == xlretSuccess;
        }
    };

}    // namespace xll

