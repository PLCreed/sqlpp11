/*
 * Copyright (c) 2013-2015, Roland Bock
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLPP_ON_H
#define SQLPP_ON_H

#include <sqlpp11/type_traits.h>
#include <sqlpp11/interpret_tuple.h>
#include <sqlpp11/interpretable_list.h>
#include <sqlpp11/unconditional.h>
#include <sqlpp11/logic.h>

namespace sqlpp
{
  template <typename Database, typename... Expressions>
  struct on_t
  {
    using _traits = make_traits<no_value_t, tag::is_on>;
    using _nodes = detail::type_vector<Expressions...>;

    using _is_dynamic = is_database<Database>;

    static_assert(_is_dynamic::value or sizeof...(Expressions), "at least one expression argument required in on()");

    template <typename Expr>
    void add(Expr expr)
    {
      static_assert(_is_dynamic::value, "on::add() must not be called for static on()");
      static_assert(is_expression_t<Expr>::value, "invalid expression argument in on::add()");
      using _serialize_check = sqlpp::serialize_check_t<typename Database::_serializer_context_t, Expr>;
      _serialize_check::_();

      using ok = logic::all_t<_is_dynamic::value, is_expression_t<Expr>::value, _serialize_check::type::value>;

      _add_impl(expr, ok());  // dispatch to prevent compile messages after the static_assert
    }

  private:
    template <typename Expr>
    void _add_impl(Expr expr, const std::true_type&)
    {
      return _dynamic_expressions.emplace_back(expr);
    }

    template <typename Expr>
    void _add_impl(Expr expr, const std::false_type&);

  public:
    std::tuple<Expressions...> _expressions;
    interpretable_list_t<Database> _dynamic_expressions;
  };

  template <>
  struct on_t<void, unconditional_t>
  {
    using _traits = make_traits<no_value_t, tag::is_on>;
    using _nodes = detail::type_vector<>;
  };

  template <typename Context>
  struct serializer_t<Context, on_t<void, unconditional_t>>
  {
    using _serialize_check = consistent_t;
    using T = on_t<void, unconditional_t>;

    static Context& _(const T&, Context& context)
    {
      return context;
    }
  };

  template <typename Context, typename Database, typename... Expressions>
  struct serializer_t<Context, on_t<Database, Expressions...>>
  {
    using _serialize_check = serialize_check_of<Context, Expressions...>;
    using T = on_t<Database, Expressions...>;

    static Context& _(const T& t, Context& context)
    {
      if (sizeof...(Expressions) == 0 and t._dynamic_expressions.empty())
        return context;
      context << " ON (";
      interpret_tuple(t._expressions, " AND ", context);
      if (sizeof...(Expressions) and not t._dynamic_expressions.empty())
        context << " AND ";
      interpret_list(t._dynamic_expressions, " AND ", context);
      context << " )";
      return context;
    }
  };
}

#endif
