//
// Created by kenne on 21/03/2025.
//

#pragma once

#include <functional>

namespace xll {

	class BeforeOpen {};
	class AfterOpen {};

	class BeforeClose {};
	class AfterClose {};

	class Add {};
	class Remove {};

	// Register macros to be called in xlAuto functions.
	template<class T>
	class Auto {
		using macro = std::function<int()>;
		inline static std::vector<macro> macros;
	public:
		Auto(macro&& m)
		{
			macros.emplace_back(m);
		}
		static int Execute(void)
		{
			for (const auto& m : macros) {
				if (!m()) {
					return 0;
				}
			}

			return 1;
		}
	};
}