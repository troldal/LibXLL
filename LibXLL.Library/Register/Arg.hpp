//
// Created by kenne on 21/03/2025.
//

#pragma once

#include <utility>

#include "../Types/String.hpp"
#include "../Utils/Traits.hpp"

// #define XLL_ARG_TYPE(X)                                                      \
// X(BOOL,     "A", "A",  "short int used as logical")                          \
// X(DOUBLE,   "B", "B",  "double")                                             \
// X(CSTRING,  "C", "C%", "XCHAR* to C style NULL terminated unicode string")   \
// X(PSTRING,  "D", "D%", "XCHAR* to Pascal style byte counted unicode string") \
// X(DOUBLE_,  "E", "E",  "pointer to double")                                  \
// X(CSTRING_, "F", "F%", "reference to a null terminated unicode string")      \
// X(PSTRING_, "G", "G%", "reference to a byte counted unicode string")         \
// X(USHORT,   "H", "H",  "unsigned 2 byte int")                                \
// X(WORD,     "H", "H",  "unsigned 2 byte int")                                \
// X(SHORT,    "I", "I",  "signed 2 byte int")                                  \
// X(LONG,     "J", "J",  "signed 4 byte int")                                  \
// X(FP,       "K", "K%", "pointer to struct FP")                               \
// X(BOOL_,    "L", "L",  "reference to a boolean")                             \
// X(SHORT_,   "M", "M",  "reference to signed 2 byte int")                     \
// X(LONG_,    "N", "N",  "reference to signed 4 byte int")                     \
// X(LPOPER,   "P", "Q",  "pointer to OPER struct (never a reference type)")    \
// X(LPXLOPER, "R", "U",  "pointer to XLOPER struct")                           \
// X(VOLATILE, "!", "!",  "called every time sheet is recalced")                \
// X(UNCALCED, "#", "#",  "dereferencing uncalced cells returns old value")     \
// X(VOID,     "",  ">",  "return type to use for asynchronous functions")      \
// X(THREAD_SAFE,  "", "$", "declares function to be thread safe")              \
// X(CLUSTER_SAFE, "", "&", "declares function to be cluster safe")             \
// X(ASYNCHRONOUS, "", "X", "declares function to be asynchronous")             \


namespace xll
{
    template <typename T>
    class Arg
    {
        String m_type;
        String m_name;
        String m_help;
        // String m_init;

    public:
        Arg(String arg_name, String arg_help)
            : m_type(traits::arg_traits<T>::excel_type),
              m_name(std::move(arg_name)),
              m_help(std::move(arg_help))
        {}

        [[nodiscard]] String type() const  { return m_type; }
        [[nodiscard]] String name() const { return m_name; }
        [[nodiscard]] String help() const { return m_help; }
        // [[nodiscard]] String init() const { return m_init; }

        // [[nodiscard]] std::tuple<String, String, String, String> as_tuple() const
        // {
        //     return std::make_tuple(m_type, m_name, m_help, m_init);
        // }

    };
}    // namespace xll
















