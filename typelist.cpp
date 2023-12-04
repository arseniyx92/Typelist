#include <utility>
#include <cstddef>
#include <type_traits>
#include <algorithm>

template <typename... Types>
struct Typelist {};

template <typename... Types>
constexpr size_t size(Typelist<Types...>) {
    return sizeof...(Types);
}

template <typename... Ts, typename... Us>
constexpr bool operator==(Typelist<Ts...>, Typelist<Us...>) {
    return false;
}

template <typename... Ts>
constexpr bool operator==(Typelist<Ts...>, Typelist<Ts...>) {
    return true;
}

template <typename... Ts, typename... Us>
constexpr bool operator!=(Typelist<Ts...> first, Typelist<Ts...> second) {
    return !(first == second);
}

template <typename Head, typename... Tail>
constexpr auto pop_front(Typelist<Head, Tail...>) {
    return Typelist<Tail...>();
}

template <typename T, typename... Types>
constexpr auto push_front(Typelist<Types...>) {
    return Typelist<T, Types...>();
}

template <typename T, typename... Types>
constexpr auto push_back(Typelist<Types...>) {
    return Typelist<Types..., T>();
}

template <typename T, typename... Types>
constexpr auto operator+(std::type_identity<T>, Typelist<Types...>) {
    return Typelist<Types..., T>();
}

template <typename... Types>
constexpr auto reverse(Typelist<Types...>) { 
    return (std::type_identity<Types>() + ... + (Typelist<>()));
}

template <typename... Types>
constexpr auto pop_back(Typelist<Types...> list) {
    return reverse(pop_front(reverse(list)));
}

template <template <typename> typename F, typename... Types>
constexpr size_t find_if(Typelist<Types...>) {
    bool b[] = {F<Types>::value...};
    return std::find(b, b + sizeof...(Types), true) - b;
}

template <typename T>
struct is_same_value_helper {
    template <typename U>
    struct check {
        static constexpr bool value = std::is_same_v<T, U>;
    };
};

template <typename T, typename... Types>
constexpr size_t find(Typelist<Types...> list) {
    //return find_if<typename is_same_value_helper<T>:: template check>(list);
    bool b[] = {std::is_same_v<T, Types>...};
    return std::find(b, b + sizeof...(Types), true) - b;
}

template <typename T, typename... Types>
constexpr bool contains(Typelist<Types...> list) {
    return find<T>(list) != sizeof...(Types);
}

template <typename T>
struct at_impl;

template <size_t... Indices>
struct at_impl<std::index_sequence<Indices...>> {
    template <typename T>
    static constexpr T dummy(decltype(Indices, static_cast<void*>(nullptr))..., T*, ...);
};

template <size_t Index, typename... Types>
constexpr auto at(Typelist<Types...>) requires(Index < sizeof...(Types)) {
    return std::type_identity<decltype(
            at_impl<std::make_index_sequence<Index>>::dummy((Types*)nullptr...))>();
}

template <typename T>
struct generate_impl;

template <size_t... Indices>
struct generate_impl<std::index_sequence<Indices...>> {
    template <typename T>
    constexpr static auto helper() {
        return Typelist<std::remove_reference_t<decltype(Indices, std::declval<T>())>...>();
    }
};

template <size_t N, typename T>
constexpr auto generate() {
    return generate_impl<std::make_index_sequence<N>>:: template helper<T>();
}

template <typename... Ts, typename... Us>
constexpr auto concat(Typelist<Ts...>, Typelist<Us...>) {
    return Typelist<Ts..., Us...>();
}

template <template <typename> typename F, typename... Types>
constexpr auto transform(Typelist<Types...>) {
    return Typelist<typename F<Types>::type...>();
}

// filter
template <template <typename> typename, typename... Types>
struct ConcatinateIfFSatisfied : public Typelist<Types...> {};

template <template <typename> typename F, typename T, typename... Types>
constexpr auto operator+(std::type_identity<T>, ConcatinateIfFSatisfied<F, Types...>) requires(F<T>::value) {
    return ConcatinateIfFSatisfied<F, T, Types...>();
}

template <template <typename> typename F, typename T, typename... Types>
constexpr auto operator+(std::type_identity<T>, ConcatinateIfFSatisfied<F, Types...>) {
    return ConcatinateIfFSatisfied<F, Types...>();
}

template <template <typename> typename F, typename... Types>
constexpr auto cast_to_Typelist(ConcatinateIfFSatisfied<F, Types...>) {
    return Typelist<Types...>();
};

template <template <typename> typename F, typename... Types>
constexpr auto filter(Typelist<Types...>) {
    return cast_to_Typelist((std::type_identity<Types>() + ... + ConcatinateIfFSatisfied<F>()));
}

// unique
template <typename... Types>
struct ConcatinateIfDifferentWithFront : public Typelist<Types...> {};

template <typename T, typename Head, typename... Tail>
struct SameAsFirst {
    static constexpr bool value = false;
};

template <typename T, typename... Tail>
struct SameAsFirst<T, T, Tail...> {
    static constexpr bool value = true;
};

template <typename T, typename... Types>
constexpr auto operator+(std::type_identity<T>, ConcatinateIfDifferentWithFront<Types...>)
            requires(SameAsFirst<T, Types...>::value) {
    return ConcatinateIfDifferentWithFront<Types...>();
}

template <typename T, typename... Types>
constexpr auto operator+(std::type_identity<T>, ConcatinateIfDifferentWithFront<Types...>) {
    return ConcatinateIfDifferentWithFront<T, Types...>();
}

template <typename... Types>
constexpr auto cast_to_Typelist(ConcatinateIfDifferentWithFront<Types...>) {
    return Typelist<Types...>();
};

template <typename... Types>
constexpr auto unique(Typelist<Types...>) {
    return cast_to_Typelist((std::type_identity<Types>() + ... + ConcatinateIfDifferentWithFront()));
}

// is_sorted
struct Empty {};

template <template <typename, typename> typename Comp, typename T>
struct SortChecker {
    bool sorted = true;
};

template <template <typename, typename> typename Comp, typename U, typename T>
constexpr auto operator+(SortChecker<Comp, U> checker, std::type_identity<T>) requires(Comp<U, T>::value) {
    return SortChecker<Comp, T>{true && checker.sorted};
}

template <template <typename, typename> typename Comp, typename T>
constexpr auto operator+(SortChecker<Comp, Empty>, std::type_identity<T>) {
    return SortChecker<Comp, T>{true};
}

template <template <typename, typename> typename Comp, typename U, typename T>
constexpr auto operator+(SortChecker<Comp, U>, std::type_identity<T>) {
    return SortChecker<Comp, T>{false};
}

template <template <typename, typename> typename Comp, typename... Types>
constexpr bool is_sorted(Typelist<Types...>) {
    return (SortChecker<Comp, Empty>() + ... + std::type_identity<Types>()).sorted;
}

// merge
template <template <typename, typename> typename Comp, typename T, typename U>
struct MergeTypelists {
    using type = Typelist<>;
};

template <template <typename, typename> typename Comp, typename... Ts>
struct MergeTypelists<Comp, Typelist<Ts...>, Typelist<>> {
    using type = Typelist<Ts...>;
};

template <template <typename, typename> typename Comp, typename... Us>
struct MergeTypelists<Comp, Typelist<>, Typelist<Us...>> {
    using type = Typelist<Us...>;
};

template <template <typename, typename> typename Comp>
struct MergeTypelists<Comp, Typelist<>, Typelist<>> {
    using type = Typelist<>;
};


template <template <typename, typename> typename Comp, typename HeadT, typename... Ts, typename HeadU, typename... Us>
struct MergeTypelists<Comp, Typelist<HeadT, Ts...>, Typelist<HeadU, Us...>> {
    using type = std::conditional_t<Comp<HeadT, HeadU>::value || !(Comp<HeadU, HeadT>::value)
                , decltype(push_front<HeadT>(typename MergeTypelists<Comp, Typelist<Ts...>, Typelist<HeadU, Us...>>::type()))
                , decltype(push_front<HeadU>(typename MergeTypelists<Comp, Typelist<HeadT, Ts...>, Typelist<Us...>>::type()))>;
};

template <template <typename, typename> typename Comp, typename... Ts, typename... Us>
constexpr auto merge(Typelist<Ts...>, Typelist<Us...>) {
    return typename MergeTypelists<Comp, Typelist<Ts...>, Typelist<Us...>>::type();
}

//sort

template <size_t N, typename... Types>
constexpr auto lastK(Typelist<Types...> list) {
    if constexpr (N == 0) {
        return Typelist<Types...>();
    } else {
        return lastK<N - 1>(pop_front(list));
    }
}

template <size_t N, typename... Types>
constexpr auto firstK(Typelist<Types...> list) {
    return reverse(lastK<N>(reverse(list)));
}

template <template <typename, typename> typename Comp, typename... Types>
constexpr auto stable_sort(Typelist<Types...> list) {
    if constexpr (sizeof...(Types) <= 1) {
        return list;
    } else {
        constexpr size_t N = sizeof...(Types);
        auto first = firstK<N / 2>(list);
        auto second = lastK<N - N / 2>(list);
        return merge<Comp>(stable_sort<Comp>(first), stable_sort<Comp>(second));
    }
}


#include <type_traits>
#include <string>


template<typename L, typename R>
struct SizeComp : std::bool_constant<sizeof(L) < sizeof(R)> {};

template<size_t N, typename T=int>
constexpr bool test_at_ce = requires { at<N>(Typelist<T>{}); };

template <typename T, typename U>
constexpr bool operator==(T, U) {
    return false;
}

template <typename T>
constexpr bool operator==(T, T) {
    return true;
}
