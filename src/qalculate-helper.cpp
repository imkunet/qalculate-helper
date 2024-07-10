// SPDX-License-Identifier: GPL-3.0
/*
 * qalculate-helper.cpp
 * Copyright (C) 2024 Marko Zajc
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exchange_update_exception.h>
#include <libqalculate/Calculator.h>
#include <libqalculate/Function.h>
#include <libqalculate/MathStructure.h>
#include <libqalculate/Unit.h>
#include <libqalculate/includes.h>
#include <security_util.h>
#include <sstream>
#include <string>
#include <timeout_exception.h>
#include <vector>

using std::size_t;
using std::string;
using std::string_view;
using std::stringstream;
using std::vector;

#if __cplusplus >= 201703L
#include <string_view>

static bool ends_with(string_view str, string_view suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

#endif

const char TYPE_MESSAGE = 1;
const char TYPE_RESULT = 2;

const char LEVEL_INFO = 1;
const char LEVEL_WARNING = 2;
const char LEVEL_ERROR = 3;
const char LEVEL_UNKNOWN = 4;

const char SEPARATOR = 0;

#define COMMAND_UPDATE "update"

const unsigned long MODE_PRECISION = 1 << 0;
const unsigned long MODE_EXACT = 1 << 1;
const unsigned long MODE_NOCOLOR = 1 << 2;

struct EvaluatedResult {
  MathStructure result;
  MathStructure parsed;
};

static EvaluatedResult evaluate_single(Calculator *calc,
                                       const EvaluationOptions &eo,
                                       unsigned long line_number,
                                       const string &expression) {
  EvaluatedResult evaluated;
  if (!calc->calculate(&evaluated.result,
                       calc->unlocalizeExpression(expression), TIMEOUT_CALC, eo,
                       &evaluated.parsed))
    throw timeout_exception();

  const CalculatorMessage *message;
  while ((message = CALCULATOR->message())) {
    putchar(TYPE_MESSAGE);
    switch (message->type()) {
    case MESSAGE_INFORMATION:
      putchar(LEVEL_INFO);
      break;
    case MESSAGE_WARNING:
      putchar(LEVEL_WARNING);
      break;
    case MESSAGE_ERROR:
      putchar(LEVEL_ERROR);
      break;
    default:
      putchar(LEVEL_UNKNOWN);
      break;
    }
    printf("line %lu: ", line_number);
    fputs(message->c_message(), stdout);
    putchar(SEPARATOR);
    calc->nextMessage();
  }
  return evaluated;
}

static bool mode_set(unsigned long mode, unsigned long test) {
  return mode & test;
}

static void set_precision(Calculator *calc, unsigned long mode,
                          EvaluationOptions &eo, PrintOptions &po) {
  int precision = PRECISION_DEFAULT;

  if (mode_set(mode, MODE_EXACT)) {
    eo.approximation = APPROXIMATION_EXACT;
    po.number_fraction_format = FRACTION_DECIMAL_EXACT;

  } else if (mode_set(mode, MODE_PRECISION)) {
    precision = PRECISION_HIGH;
    po.indicate_infinite_series = false;
  }

  calc->setPrecision(precision);
}

EvaluatedResult evaluate_all(const vector<string> &expressions,
                             const EvaluationOptions &eo, Calculator *calc) {
  for (size_t i = 0; i < expressions.size() - 1; ++i)
    evaluate_single(calc, eo, i + 1, expressions[i]);
  return evaluate_single(calc, eo, expressions.size(), expressions.back());
}

// ripped from libqalculate:
// https://github.com/Qalculate/libqalculate/blob/de8021afc8649986abbbe70e82ecd16a2cc95ff4/libqalculate/Calculator-calculate.cc#L879C1-L888C2
bool test_parsed_comparison(const MathStructure &m) {
  if (m.isComparison())
    return true;
  if ((m.isLogicalOr() || m.isLogicalAnd()) && m.size() > 0) {
    for (size_t i = 0; i < m.size(); i++) {
      if (!test_parsed_comparison(m[i]))
        return false;
    }
    return true;
  }
  return false;
}

string print_raw(Calculator *calc, MathStructure structure, PrintOptions &po,
                 int mode) {
  return calc->print(structure, TIMEOUT_PRINT, po, false,
                     mode_set(mode, MODE_NOCOLOR) ? 0 : 1, TAG_TYPE_TERMINAL);
}

void print_result(Calculator *calc, const EvaluatedResult &result_struct,
                  PrintOptions &po, int mode, bool &approximate) {
  MathStructure mResult(result_struct.result);

  string parsed = print_raw(calc, result_struct.parsed, po, mode);

  // convert to true/false when necessary
  if (test_parsed_comparison(result_struct.parsed)) {
    if (mResult.isZero()) {
      Variable *v = calc->getActiveVariable("false");
      if (v)
        mResult.set(v);
    } else if (mResult.isOne()) {
      Variable *v = calc->getActiveVariable("true");
      if (v)
        mResult.set(v);
    }
  }

  string result = print_raw(calc, mResult, po, mode);
  // po.number_fraction_format = FRACTION_DECIMAL;
  // string result_decimal = print_raw(calc, result_struct.result, po, mode);

  if (ends_with(result, calc->timedOutString())) {
    throw timeout_exception();
  } else {
    putchar(TYPE_RESULT);
    if (parsed.compare(result)) {
      fputs(parsed.c_str(), stdout);
      fputs(" ", stdout);
    }
    fputs(approximate ? "≈ " : "= ", stdout);
    fputs(result.c_str(), stdout);
    /*if (result.compare(result_decimal)) {
      fputs(" ≈ ", stdout);
      fputs(result_decimal.c_str(), stdout);
    }*/
  }
  putchar(SEPARATOR);
}

static EvaluationOptions get_evaluationoptions() {
  EvaluationOptions eo;
  eo.approximation = APPROXIMATION_TRY_EXACT;
  eo.parse_options.unknowns_enabled = false;
  // eo.sync_units = false; // causes issues with monetary conversion, eg x usd
  // > 1 eur. no idea why I enabled this in the first place
  return eo;
}

static PrintOptions get_printoptions(int base, bool *approximate) {
  PrintOptions po;
  po.base = base;
  po.number_fraction_format = FRACTION_DECIMAL;
  po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
  po.use_unicode_signs = true;
  // po.min_decimals = MIN_DECIMALS;
  po.time_zone = TIME_ZONE_UTC;
  po.abbreviate_names = true;
  po.spell_out_logical_operators = true;
  po.allow_non_usable = true;
  po.show_ending_zeroes = false;
  // po.preserve_precision = true;
  // po.restrict_to_parent_precision = false;
  po.is_approximate = approximate;
  return po;
}

static void evaluate(Calculator *calc, const vector<string> &expressions,
                     unsigned int mode, int base) {
  calc->setExchangeRatesWarningEnabled(false);
  calc->loadExchangeRates();
  calc->loadGlobalDefinitions();

  calc->addUnit(new AliasUnit("Minecraft Tick", "tick", "ticks", "tick", "Tick",
                              calc->getUnitById(UNIT_ID_SECOND), "0.05", 1, "",
                              true, true, true));
  calc->addFunction(
      new UserFunction("", "snowstamp", "unix2date((\\x>>22)/1000+1420070400)",
                       true, 1, "Discord Snowflake to time",
                       "Converts a Discord Snowflake to a time", 1, true));

  bool approximate = false;

  PrintOptions po = get_printoptions(base, &approximate);
  EvaluationOptions eo = get_evaluationoptions();
  set_precision(calc, mode, eo, po);

  calc->setMessagePrintOptions(po);

  do_seccomp();

  auto result_struct = evaluate_all(expressions, eo, calc);

  print_result(calc, result_struct, po, mode, approximate);
}

static void update() {
  if (!CALCULATOR->canFetch())
    throw exchange_update_exception();

  CALCULATOR->fetchExchangeRates(TIMEOUT_UPDATE);
  // for some reason this returns false even when it's successful so I'm not
  // going to check it
}

static vector<string> parseExpressions(stringstream input) {
  vector<string> result;
  string expression;
  while (std::getline(input, expression, '\n'))
    result.push_back(expression);

  return result;
}

int main(int argc, char **argv) {
  do_setuid();

  if (argc < 2)
    return 1;

  auto *calc = new Calculator(true);

#ifndef SKIP_DEFANG
  do_defang_calculator(calc);
#endif

  try {
    if (argc == 2) {
      if (strcmp(argv[1], COMMAND_UPDATE) == 0)
        update();
      else
        return 1;
    } else {
      evaluate(calc, parseExpressions(stringstream(argv[1])),
               std::strtoul(argv[2], nullptr, 10),
               std::strtol(argv[3], nullptr, 10));
    }

  } catch (const qalculate_exception &e) {
    return e.getCode();
  }
}
