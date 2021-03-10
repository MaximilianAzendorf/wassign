/*
 * Copyright 2020 Maximilian Azendorf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "../Types.h"
#include "Tagged.h"
#include "InputReader.h"
#include "ConstraintExpression.h"

#include <rapidcsv.h>

class InputReader;

class ChaiscriptInterface
{
private:
    ChaiscriptInterface() = default;

    template<typename T>
    static string find_by_name(string const& name, map<string, T>& values);

public:
    static void register_interface(InputReader& reader);

    static shared_ptr<InputSlotData> slot(InputReader& reader, string const& name);
    static shared_ptr<InputSlotData> slot(InputReader& reader, string const& name, vector<Tagged> const& t);

    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1, Tagged const& t2);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1, Tagged const& t2, Tagged const& t3);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1, Tagged const& t2, Tagged const& t3, Tagged const& t4);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              vector<Tagged> const& t);

    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name);
    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name,
                                                vector<int> const& preferences);
    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name,
                                                vector<Tagged> const& t, vector<int> const& preferences);
    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name,
                                                vector<string> const& preferences);
    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name,
                                                vector<Tagged> const& t, vector<string> const& preferences);

    static ConstraintExpression constraint(ConstraintExpression constraintExpression);

    static shared_ptr<InputSlotData> add(InputReader& reader, shared_ptr<InputSlotData> slot);
    static shared_ptr<InputChoiceData> add(InputReader& reader, shared_ptr<InputChoiceData> choice);
    static shared_ptr<InputChooserData> add(InputReader& reader, shared_ptr<InputChooserData> chooser);
    static ConstraintExpression add(InputReader& reader, ConstraintExpression constraintExpression);

    static ConstraintExpressionAccessor cexp_choices(shared_ptr<InputSlotData> const& slot);
    static ConstraintExpressionAccessor cexp_choices(shared_ptr<InputChooserData> const& chooser);
    static ConstraintExpressionAccessor cexp_slot(shared_ptr<InputChoiceData> const& choice);
    static ConstraintExpressionAccessor cexp_slot(shared_ptr<InputChoiceData> const& choice, int part);
    static ConstraintExpressionAccessor cexp_choosers(shared_ptr<InputChoiceData> const& choice);
    static ConstraintExpressionAccessor cexp_choosers(shared_ptr<InputChoiceData> const& choice, int part);
    static ConstraintExpressionAccessor cexp_size(shared_ptr<InputSlotData> const& slot);

    static ConstraintExpression cexp_eq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_neq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_lt(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_gt(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_leq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_geq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_contains(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_contains_not(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_subset(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);
    static ConstraintExpression cexp_superset(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right);

    static ConstraintExpressionAccessor cexp_slot_accessor_conversion(InputSlotData const& set);
    static ConstraintExpressionAccessor cexp_choice_accessor_conversion(InputChoiceData const& choice);
    static ConstraintExpressionAccessor cexp_chooser_accessor_conversion(InputChooserData const& chooser);
    static ConstraintExpressionAccessor cexp_integer_accessor_conversion(int const& integer);

    static int int_string_conversion(string const& string);
    static string string_int_conversion(int n);

    static string string_int_append(string const& s, int n);
    static string int_string_append(int n, string const& s);

    static vector<int> range(int from, int to);

    static string read_file_string(string const& filename);
    static shared_ptr<rapidcsv::Document> read_file_csv(string const& filename);
    static shared_ptr<rapidcsv::Document> read_file_csv(string const& filename, char separator);

    static vector<string> get_csv_row(rapidcsv::Document const& doc, int index);
    static vector<vector<string>> get_csv_rows(rapidcsv::Document const& doc);

    static inline const int end = INT_MAX;
    static vector<string> slice(vector<string> const& v, int from, int to);

    static void set_arguments(InputReader& reader, vector<string> args);

    static inline const Tagged optional_obj = Tagged(Optional, 1);

    static Tagged min(int min);
    static Tagged max(int max);
    static Tagged bounds(int min, int max);
    static Tagged parts(int parts);
    static Tagged optional(bool value);
};

#include "ChaiscriptInterface.ipp"


