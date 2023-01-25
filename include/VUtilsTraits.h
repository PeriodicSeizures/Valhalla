#pragma once

//#include "rpc/detail/bool.h"
// Borrowed from
//  https://github.com/rpclib/rpclib/blob/master/include/rpc/detail/func_traits.h

namespace VUtils {
    namespace Traits {

        template<typename T>
        using invoke = typename T::type;

        //template <int N>
        //using is_zero = invoke<std::conditional<(N == 0), true_, false_>>;

        template <int N, typename... Ts>
        using nth_type = invoke<std::tuple_element<N, std::tuple<Ts...>>>;

        namespace tags {

            // tags for the function traits, used for tag dispatching
            struct zero_arg {};
            struct nonzero_arg {};
            struct void_result {};
            struct nonvoid_result {};

            template <int N> struct arg_count_trait { typedef nonzero_arg type; };

            template <> struct arg_count_trait<0> { typedef zero_arg type; };

            template <typename T> struct result_trait { typedef nonvoid_result type; };

            template <> struct result_trait<void> { typedef void_result type; };
        }

        //! \brief Provides a small function traits implementation that
        //! works with a reasonably large set of functors.
        template <typename T>
        struct func_traits : func_traits<decltype(&T::operator())> {};

        template <typename C, typename R, typename... Args>
        struct func_traits<R(C::*)(Args...)> : func_traits<R(*)(Args...)> {};

        template <typename C, typename R, typename... Args>
        struct func_traits<R(C::*)(Args...) const> : func_traits<R(*)(Args...)> {};

        template <typename R, typename... Args> struct func_traits<R(*)(Args...)> {
            using result_type = R;
            using arg_count = std::integral_constant<std::size_t, sizeof...(Args)>;
            using args_type = std::tuple<typename std::decay<Args>::type...>;
        };



        // get type in lambda
        //template <typename F>
        //struct lam_nth_arg



        template <typename T>
        struct func_kind_info : func_kind_info<decltype(&T::operator())> {};

        template <typename C, typename R, typename... Args>
        struct func_kind_info<R(C::*)(Args...)> : func_kind_info<R(*)(Args...)> {};

        template <typename C, typename R, typename... Args>
        struct func_kind_info<R(C::*)(Args...) const>
            : func_kind_info<R(*)(Args...)> {};

        template <typename R, typename... Args> struct func_kind_info<R(*)(Args...)> {
            typedef typename tags::arg_count_trait<sizeof...(Args)>::type args_kind;
            typedef typename tags::result_trait<R>::type result_kind;
        };

        //template <typename F> using is_zero_arg = is_zero<func_traits<F>::arg_count>;

        //template <typename F>
        //using is_single_arg =
            //invoke<std::conditional<func_traits<F>::arg_count == 1, true_, false_>>;

        template <typename F>
        using is_void_result = std::is_void<typename func_traits<F>::result_type>;
    }
}
