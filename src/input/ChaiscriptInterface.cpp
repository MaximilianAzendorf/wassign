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

#include "ChaiscriptInterface.h"
#include "../Util.h"

#include <utility>

#include <chaiscript/extras/math.hpp>
#include <chaiscript/extras/string_methods.hpp>

void unsupported_value(Tagged const& t)
{
    throw InputException("Unsupported value '" + Tagged::tag_name(t.tag()) + "(...)'.");
}

void ChaiscriptInterface::register_interface(InputReader& reader)
{
    auto& c = reader._chai;
    auto readerRef = std::ref(reader);

    c.add(cs::extras::math::bootstrap());
    c.add(cs::extras::string_methods::bootstrap());

    c.add(cs::bootstrap::standard_library::vector_type<vector<int>>("vectorInt"));
    c.add(cs::bootstrap::standard_library::vector_type<vector<string>>("vectorString"));
    c.add(cs::bootstrap::standard_library::vector_type<vector<vector<string>>>("vectorString2d"));

    c.add(cs::vector_conversion<vector<int>>());
    c.add(cs::vector_conversion<vector<string>>());
    c.add(cs::vector_conversion<vector<vector<string>>>());
    c.add(cs::vector_conversion<vector<Tagged>>());

    c.add_global_const(chaiscript::const_var(&ChaiscriptInterface::end), "end");
    c.add(cs::fun(&ChaiscriptInterface::slice), "slice");

    c.add(cs::fun(&ChaiscriptInterface::set_arguments, readerRef), "set_arguments");

    c.add(cs::fun(&ChaiscriptInterface::range), "range");

    c.add(cs::fun(&ChaiscriptInterface::string_int_append), "+");
    c.add(cs::fun(&ChaiscriptInterface::int_string_append), "+");

    c.add(cs::user_type<ConstraintExpression>(), "__cexp");
    c.add(cs::user_type<ConstraintExpressionAccessor>(), "__cexpAccessor");

    c.add(cs::user_type<InputSlotData>(), "slot");
    c.add(cs::user_type<SlotData>(), "rawSlot");
    c.add(cs::fun(&SlotData::name), "name");

    c.add(cs::user_type<InputChoiceData>(), "choice");
    c.add(cs::user_type<ProtoChoiceData>(), "rawChoice");
    c.add(cs::fun(&ProtoChoiceData::name), "name");
    c.add(cs::fun(&ProtoChoiceData::min), "min");
    c.add(cs::fun(&ProtoChoiceData::max), "max");
    c.add(cs::fun(&ProtoChoiceData::parts), "parts");
    c.add(cs::fun(&ProtoChoiceData::optional), "optional");

    c.add(cs::user_type<InputChooserData>(), "chooser");
    c.add(cs::user_type<ChooserData>(), "rawChooser");
    c.add(cs::fun(&ChooserData::name), "name");
    c.add(cs::fun(&ChooserData::preferences), "preferences");

    c.add(cs::user_type<rapidcsv::Document>(), "csvDoc");
    c.add(cs::fun(&ChaiscriptInterface::get_csv_rows), "rows");
    c.add(cs::fun(&ChaiscriptInterface::get_csv_row), "row");
    c.add(cs::fun(&ChaiscriptInterface::get_csv_row), "[]");

    c.add(cs::base_class<SlotData, InputSlotData>());
    c.add(cs::base_class<ProtoChoiceData, InputChoiceData>());
    c.add(cs::base_class<ChooserData, InputChooserData>());

    c.add(cs::type_conversion<InputSlotData, ConstraintExpressionAccessor>(&ChaiscriptInterface::cexp_slot_accessor_conversion));
    c.add(cs::type_conversion<InputChoiceData, ConstraintExpressionAccessor>(&ChaiscriptInterface::cexp_choice_accessor_conversion));
    c.add(cs::type_conversion<InputChooserData, ConstraintExpressionAccessor>(&ChaiscriptInterface::cexp_chooser_accessor_conversion));
    c.add(cs::type_conversion<int, ConstraintExpressionAccessor>(&ChaiscriptInterface::cexp_integer_accessor_conversion));
    c.add(cs::type_conversion<string, int>(&ChaiscriptInterface::int_string_conversion));
    c.add(cs::type_conversion<int, string>(&ChaiscriptInterface::string_int_conversion));

    c.add(cs::fun(static_cast<shared_ptr<InputSlotData> (*)(InputReader&, string const&)>(&ChaiscriptInterface::slot), readerRef), "slot");
    c.add(cs::fun(static_cast<shared_ptr<InputSlotData> (*)(InputReader&, string const&, vector<Tagged> const&)>(&ChaiscriptInterface::slot), readerRef), "slot");

    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, vector<Tagged> const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&, Tagged const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&, Tagged const&, Tagged const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");

    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&, vector<int> const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&, vector<Tagged> const& t, vector<int> const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&, vector<string> const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&, vector<Tagged> const& t, vector<string> const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");

    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputSlotData> const&)>(&ChaiscriptInterface::cexp_choices)), "choices");
    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputChooserData> const&)>(&ChaiscriptInterface::cexp_choices)), "choices");
    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputChoiceData> const&)>(&ChaiscriptInterface::cexp_slot)), "slot");
    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputChoiceData> const&, int)>(&ChaiscriptInterface::cexp_slot)), "slot");
    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputChoiceData> const&)>(&ChaiscriptInterface::cexp_choosers)), "choosers");
    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputChoiceData> const&, int)>(&ChaiscriptInterface::cexp_choosers)), "choosers");
    c.add(cs::fun(static_cast<ConstraintExpressionAccessor (*)(shared_ptr<InputSlotData> const&)>(&ChaiscriptInterface::cexp_size)), "size");

    c.add(cs::fun(&ChaiscriptInterface::constraint), "constraint");

    c.add(cs::fun(static_cast<shared_ptr<InputSlotData> (*)(InputReader&, shared_ptr<InputSlotData>)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<shared_ptr<InputSlotData> (*)(InputReader&, shared_ptr<InputSlotData>)>(&ChaiscriptInterface::add), readerRef), "+");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, shared_ptr<InputChoiceData>)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, shared_ptr<InputChoiceData>)>(&ChaiscriptInterface::add), readerRef), "+");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, shared_ptr<InputChooserData>)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, shared_ptr<InputChooserData>)>(&ChaiscriptInterface::add), readerRef), "+");
    c.add(cs::fun(static_cast<ConstraintExpression (*)(InputReader&, ConstraintExpression)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<ConstraintExpression (*)(InputReader&, ConstraintExpression)>(&ChaiscriptInterface::add), readerRef), "+");

    c.add(cs::fun(&ChaiscriptInterface::cexp_eq), "==");
    c.add(cs::fun(&ChaiscriptInterface::cexp_neq), "!=");
    c.add(cs::fun(&ChaiscriptInterface::cexp_lt), "<");
    c.add(cs::fun(&ChaiscriptInterface::cexp_gt), ">");
    c.add(cs::fun(&ChaiscriptInterface::cexp_leq), "<=");
    c.add(cs::fun(&ChaiscriptInterface::cexp_geq), ">=");
    c.add(cs::fun(&ChaiscriptInterface::cexp_contains), "contains");
    c.add(cs::fun(&ChaiscriptInterface::cexp_contains_not), "contains_not");
    c.add(cs::fun(&ChaiscriptInterface::cexp_subset), "subsetOf");
    c.add(cs::fun(&ChaiscriptInterface::cexp_superset), "supersetOf");

    c.add(cs::fun(&ChaiscriptInterface::read_file_string), "readFile");
    c.add(cs::fun(static_cast<shared_ptr<rapidcsv::Document> (*)(string const&)>(&ChaiscriptInterface::read_file_csv)), "readCsv");
    c.add(cs::fun(static_cast<shared_ptr<rapidcsv::Document> (*)(string const&, char)>(&ChaiscriptInterface::read_file_csv)), "readCsv");

    c.add_global_const(chaiscript::const_var(&ChaiscriptInterface::optional), "optional");
    c.add(cs::fun(&ChaiscriptInterface::min), "min");
    c.add(cs::fun(&ChaiscriptInterface::max), "max");
    c.add(cs::fun(&ChaiscriptInterface::parts), "parts");
    c.add(cs::fun(&ChaiscriptInterface::bounds), "bounds");
}

shared_ptr<InputSlotData> ChaiscriptInterface::slot(InputReader& reader, string const& name)
{
    string foundName = find_by_name(name, reader._setMap);

    if(!foundName.empty())
    {
        return reader._setMap[foundName];
    }

    return slot(reader, name, {});
}

shared_ptr<InputSlotData> ChaiscriptInterface::slot(InputReader& reader, string const& name, vector<Tagged> const& t)
{
    auto newSet = std::make_shared<InputSlotData>();
    newSet->registered = false;
    newSet->name = name;
    reader._inputObjects.push_back(newSet);

    for(auto const& tagged : t) unsupported_value(tagged);

    return newSet;
}

shared_ptr<InputChoiceData> ChaiscriptInterface::choice(InputReader& reader, string const& name)
{
    string foundName = find_by_name(name, reader._choiceMap);

    if(!foundName.empty())
    {
        return reader._choiceMap[foundName];
    }

    return choice(reader, name, {});
}

shared_ptr<InputChoiceData> ChaiscriptInterface::choice(InputReader& reader,
                                                        string const& name,
                                                        vector<Tagged> const& t)
{
    auto newChoice = std::make_shared<InputChoiceData>();
    newChoice->registered = false;
    newChoice->name = name;
    newChoice->parts = 1;
    newChoice->min = 1;
    newChoice->max = 1;
    newChoice->optional = false;
    reader._inputObjects.push_back(newChoice);

    for(auto const& tagged : t)
    {
        switch(tagged.tag())
        {
            case Ignore: break;
            case Min: newChoice->min = tagged.value(); break;
            case Max: newChoice->max = tagged.value(); break;
            case Optional: newChoice->optional = true; break;
            case Parts: newChoice->parts = tagged.value(); break;
            case Bounds: newChoice->min = tagged.value(0); newChoice->max = tagged.value(1); break;
            default: unsupported_value(tagged);
        }
    }

    return newChoice;
}

shared_ptr<InputChoiceData>
ChaiscriptInterface::choice(InputReader& reader,
                            string const& name,
                            Tagged const& t1)
{
    return choice(reader, name, vector<Tagged>{t1});
}

shared_ptr<InputChoiceData>
ChaiscriptInterface::choice(InputReader& reader,
                            string const& name,
                            Tagged const& t1,
                            Tagged const& t2)
{
    return choice(reader, name, {t1, t2});
}

shared_ptr<InputChoiceData>
ChaiscriptInterface::choice(InputReader& reader,
                            string const& name,
                            Tagged const& t1,
                            Tagged const& t2,
                            Tagged const& t3)
{
    return choice(reader, name, {t1, t2, t3});
}

shared_ptr<InputChoiceData>
ChaiscriptInterface::choice(InputReader& reader,
                            string const& name,
                            Tagged const& t1,
                            Tagged const& t2,
                            Tagged const& t3,
                            Tagged const& t4)
{
    return choice(reader, name, {t1, t2, t3, t4});
}

shared_ptr<InputChooserData> ChaiscriptInterface::chooser(InputReader& reader, string const& name)
{
    string foundName = find_by_name(name, reader._chooserMap);

    if(!foundName.empty())
    {
        return reader._chooserMap[foundName];
    }

    return chooser(reader, name, vector<int>());
}

shared_ptr<InputChooserData>
ChaiscriptInterface::chooser(InputReader& reader, string const& name, vector<int> const& preferences)
{
    return chooser(reader, name, vector<Tagged>{}, preferences);
}

shared_ptr<InputChooserData>
ChaiscriptInterface::chooser(InputReader& reader, string const& name, vector<Tagged> const& t,
                             vector<int> const& preferences)
{
    auto newChooser = std::make_shared<InputChooserData>();
    newChooser->registered = false;
    newChooser->name = name;
    newChooser->preferences = preferences;
    reader._inputObjects.push_back(newChooser);

    for(auto const& tagged : t) unsupported_value(tagged);

    return newChooser;
}

shared_ptr<InputChooserData>
ChaiscriptInterface::chooser(InputReader& reader, string const& name, vector<string> const& preferences)
{
    return chooser(reader, name, vector<Tagged>{}, parse_ints(preferences));
}

shared_ptr<InputChooserData>
ChaiscriptInterface::chooser(InputReader& reader, string const& name, vector<Tagged> const& t,
                             vector<string> const& preferences)
{
    return chooser(reader, name, t, parse_ints(preferences));
}

ConstraintExpression ChaiscriptInterface::constraint(ConstraintExpression constraintExpression)
{
    return constraintExpression;
}


shared_ptr<InputSlotData> ChaiscriptInterface::add(InputReader& reader, shared_ptr<InputSlotData> set)
{
    if(reader._setMap.find(set->name) != reader._setMap.end())
    {
        throw InputException("Duplicate set name '" + set->name + "'.");
    }

    set->registered = true;
    reader._setMap[set->name] = set;
    reader._sets.push_back(set);

    return set;
}

shared_ptr<InputChoiceData> ChaiscriptInterface::add(InputReader& reader, shared_ptr<InputChoiceData> choice)
{
    if(reader._setMap.find(choice->name) != reader._setMap.end())
    {
        throw InputException("Duplicate choice name '" + choice->name + "'.");
    }

    choice->registered = true;
    reader._choiceMap[choice->name] = choice;
    reader._choices.push_back(choice);

    return choice;
}

shared_ptr<InputChooserData> ChaiscriptInterface::add(InputReader& reader, shared_ptr<InputChooserData> chooser)
{
    if(reader._setMap.find(chooser->name) != reader._setMap.end())
    {
        throw InputException("Duplicate chooser name '" + chooser->name + "'.");
    }

    chooser->registered = true;
    reader._chooserMap[chooser->name] = chooser;
    reader._choosers.push_back(chooser);

    return chooser;
}

ConstraintExpression ChaiscriptInterface::add(InputReader& reader, ConstraintExpression constraintExpression)
{
    reader._constraintExpressions.push_back(constraintExpression);
    return constraintExpression;
}

Tagged ChaiscriptInterface::min(int min)
{
    return Tagged(Min, min);
}

Tagged ChaiscriptInterface::max(int max)
{
    return Tagged(Max, max);
}

Tagged ChaiscriptInterface::bounds(int min, int max)
{
    return Tagged(Bounds, {min, max});
}

Tagged ChaiscriptInterface::parts(int parts)
{
    return Tagged(Parts, parts);
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_choices(shared_ptr<InputSlotData> const& set)
{
    return { .type = Slot, .subType = Choice, .name = set->name, .part = 0 };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_choices(shared_ptr<InputChooserData> const& chooser)
{
    return { .type = Chooser, .subType = Choice, .name = chooser->name, .part = 0 };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_slot(shared_ptr<InputChoiceData> const& choice)
{
    return cexp_slot(choice, 0);
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_slot(shared_ptr<InputChoiceData> const& choice, int part)
{
    return { .type = Choice, .subType = Slot, .name = choice->name, .part = part };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_choosers(shared_ptr<InputChoiceData> const& choice)
{
    return cexp_choosers(choice, 0);
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_choosers(shared_ptr<InputChoiceData> const& choice, int part)
{
    return { .type = Choice, .subType = Chooser, .name = choice->name, .part = part };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_size(shared_ptr<InputSlotData> const& set)
{
    return { .type = Slot, .subType = Size, .name = set->name, .part = 0 };
}

ConstraintExpression ChaiscriptInterface::cexp_eq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = REq}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_neq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RNeq}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_lt(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RLt}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_gt(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RGt}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_leq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RLeq}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_geq(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RGeq}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_contains(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RContains}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_contains_not(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RNotContains}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_subset(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RSubset}, .right = std::move(right)};
}

ConstraintExpression ChaiscriptInterface::cexp_superset(ConstraintExpressionAccessor left, ConstraintExpressionAccessor right)
{
    return {.left = std::move(left), .relation = {.type = RSuperset}, .right = std::move(right)};
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_slot_accessor_conversion(InputSlotData const& set)
{
    return { .type = Slot, .subType = NotSet, .name = set.name, .part = 0 };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_choice_accessor_conversion(InputChoiceData const& choice)
{
    return { .type = Choice, .subType = NotSet, .name = choice.name, .part = 0 };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_chooser_accessor_conversion(InputChooserData const& chooser)
{
    return { .type = Chooser, .subType = NotSet, .name = chooser.name, .part = 0 };
}

ConstraintExpressionAccessor ChaiscriptInterface::cexp_integer_accessor_conversion(int const& integer)
{
    return { .type = Integer, .subType = NotSet, .name = str(integer), .part = 0 };
}

int ChaiscriptInterface::int_string_conversion(string const& string)
{
    return std::stoi(string);
}

string ChaiscriptInterface::read_file_string(string const& filename)
{
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

vector<string> ChaiscriptInterface::get_csv_row(rapidcsv::Document const& doc, int index)
{
    return doc.GetRow<string>(index);
}

vector<vector<string>> ChaiscriptInterface::get_csv_rows(rapidcsv::Document const& doc)
{
    vector<vector<string>> rows;

    for(int i = 0; i < doc.GetRowCount(); i++)
    {
        rows.push_back(doc.GetRow<string>(i));
    }

    return rows;
}

shared_ptr<rapidcsv::Document> ChaiscriptInterface::read_file_csv(string const& filename)
{
    return read_file_csv(filename, ',');
}

shared_ptr<rapidcsv::Document> ChaiscriptInterface::read_file_csv(string const& filename, char separator)
{
    return std::make_shared<rapidcsv::Document>(filename, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(separator));
}

vector<string> ChaiscriptInterface::slice(vector<string> const& v, int from, int to)
{
    vector<string> slice;

    if(to == end) to = v.size() - 1;

    for(int i = from; i <= to; i++)
    {
        slice.push_back(v[i]);
    }

    return slice;
}

void ChaiscriptInterface::set_arguments(InputReader& reader, vector<string> args)
{
    args.insert(args.begin(), "");
    vector<char*> ptrVector(args.size());
    for(int i = 0; i < args.size(); i++)
    {
        ptrVector[i] = args[i].data();
    }

    reader._options->parse_override(args.size(), ptrVector.data());
}

string ChaiscriptInterface::string_int_conversion(int n)
{
    return str(n);
}

string ChaiscriptInterface::string_int_append(string const& s, int n)
{
    return s + str(n);
}

string ChaiscriptInterface::int_string_append(int n, string const& s)
{
    return str(n) + s;
}

vector<int> ChaiscriptInterface::range(int from, int to)
{
    vector<int> res;

    for(int i = from; i <= to; i++)
    {
        res.push_back(i);
    }

    return res;
}
