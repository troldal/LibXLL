//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "../Types/String.hpp"
#include <xlcall.h>

#include <utility>

namespace xll
{

    class Alert
    {
    public:

        enum Type { None = 0, Question = 1, Information = 2, Error = 3 };

        explicit Alert(String msg, Type type = None) : m_message(std::move(msg)), m_type(type) {}

        void operator()()
        {
            if (m_type == None)
                Excel12(xlcAlert, nullptr, 1, &m_message);
            else {
                auto type = xll::Int((int)m_type);
                Excel12(xlcAlert, nullptr, 2, &m_message, &type);
            }
        }

    private:
        String m_message;
        Type m_type = None;
    };

    inline void alert(String msg, Alert::Type type = Alert::None)
    {
        auto cmd = Alert(std::move(msg), type);
        cmd();
    }

}    // namespace xll