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

void unsupported_value(Tagged const& t)
{
    throw InputException("Unsupported value '" + Tagged::tag_name(t.tag()) + "(...)'.");
}

void ChaiscriptInterface::register_interface(InputReader& reader)
{
    auto& c = reader._chai;
    auto readerRef = std::ref(reader);

    c.add(cs::vector_conversion<vector<int>>());
    c.add(cs::vector_conversion<vector<Tagged>>());

    c.add(cs::user_type<InputSetData>(), "set");
    c.add(cs::user_type<SetData>(), "rawSet");
    c.add(cs::fun(&SetData::name), "name");

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

    c.add(cs::base_class<SetData, InputSetData>());
    c.add(cs::base_class<ProtoChoiceData, InputChoiceData>());
    c.add(cs::base_class<ChooserData, InputChooserData>());

    c.add(cs::fun(static_cast<shared_ptr<InputSetData> (*)(InputReader&, string const&)>(&ChaiscriptInterface::set), readerRef), "set");
    c.add(cs::fun(static_cast<shared_ptr<InputSetData> (*)(InputReader&, string const&, vector<Tagged> const&)>(&ChaiscriptInterface::set), readerRef), "set");

    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, vector<Tagged> const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&, Tagged const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, string const&, Tagged const&, Tagged const&, Tagged const&, Tagged const&)>(&ChaiscriptInterface::choice), readerRef), "choice");

    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&, vector<int> const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, string const&, vector<Tagged> const& t, vector<int> const&)>(&ChaiscriptInterface::chooser), readerRef), "chooser");

    c.add(cs::fun(static_cast<shared_ptr<InputSetData> (*)(InputReader&, shared_ptr<InputSetData>)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<shared_ptr<InputSetData> (*)(InputReader&, shared_ptr<InputSetData>)>(&ChaiscriptInterface::add), readerRef), "+");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, shared_ptr<InputChoiceData>)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<shared_ptr<InputChoiceData> (*)(InputReader&, shared_ptr<InputChoiceData>)>(&ChaiscriptInterface::add), readerRef), "+");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, shared_ptr<InputChooserData>)>(&ChaiscriptInterface::add), readerRef), "add");
    c.add(cs::fun(static_cast<shared_ptr<InputChooserData> (*)(InputReader&, shared_ptr<InputChooserData>)>(&ChaiscriptInterface::add), readerRef), "+");

    c.add(cs::fun(&ChaiscriptInterface::min), "min");
    c.add(cs::fun(&ChaiscriptInterface::max), "max");
    c.add(cs::fun(&ChaiscriptInterface::parts), "parts");
    c.add(cs::fun(&ChaiscriptInterface::bounds), "bounds");
    c.add(cs::fun(static_cast<Tagged (*)()>(&ChaiscriptInterface::optional)), "optional");
    c.add(cs::fun(static_cast<Tagged (*)(bool)>(&ChaiscriptInterface::optional)), "optional");

    c.add(cs::fun(&ChaiscriptInterface::add_constraint, readerRef), "add_constraint");
}

shared_ptr<InputSetData> ChaiscriptInterface::set(InputReader& reader, string const& name)
{
    string foundName = find_by_name(name, reader._setMap);

    if(!foundName.empty())
    {
        return reader._setMap[foundName];
    }

    return set(reader, name, {});
}

shared_ptr<InputSetData> ChaiscriptInterface::set(InputReader& reader, string const& name, vector<Tagged> const& t)
{
    auto newSet = std::make_shared<InputSetData>();
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

    return chooser(reader, name, {});
}

shared_ptr<InputChooserData>
ChaiscriptInterface::chooser(InputReader& reader, string const& name, vector<int> const& preferences)
{
    (void)reader;

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

shared_ptr<InputSetData> ChaiscriptInterface::add(InputReader& reader, shared_ptr<InputSetData> set)
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

Tagged ChaiscriptInterface::optional()
{
    return Tagged(Optional, 1);
}

Tagged ChaiscriptInterface::optional(bool optional)
{
    return Tagged(Optional, optional ? 1 : 0);
}

void ChaiscriptInterface::add_constraint(InputReader& reader, string const& constraintStr)
{
    reader._constraintStrings.push_back(constraintStr);
}
