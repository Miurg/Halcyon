#pragma once
#include <tuple>
#include <type_traits>

// Проверка наличия using Dependencies = ...
template <typename T, typename = void>
struct has_dependencies : std::false_type
{
};

template <typename T>
struct has_dependencies<T, std::void_t<typename T::Dependencies>> : std::true_type
{
};

// Проверка наличия static constexpr bool is_mandatory
template <typename T, typename = void>
struct has_mandatory_flag : std::false_type
{
};

template <typename T>
struct has_mandatory_flag<T, std::void_t<decltype(T::is_mandatory)>> : std::true_type
{
};

// Хелпер для получения значения is_mandatory (по умолчанию false)
template <typename T>
constexpr bool get_mandatory_val()
{
	if constexpr (has_mandatory_flag<T>::value)
		return T::is_mandatory;
	else
		return false;
}