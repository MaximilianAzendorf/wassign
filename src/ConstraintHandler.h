/*
 * Copyright 2021 Maximilian Azendorf
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

#include "Constraint.h"

/**
 * This class is used to process different types of constraints through given constraint handler functions.
 */
template<typename HFuncRet, typename HThis, typename... HFuncArgs>
class ConstraintHandler
{
private:
    typedef HFuncRet (HThis::* HFunc)(Constraint, HFuncArgs...);
    map<ConstraintType, HFunc> _handlers;

    HFunc* find_handler(ConstraintType type);

public:
    /**
     * Constructor
     */
     ConstraintHandler(std::initializer_list<pair<ConstraintType, HFunc>> initializerList);

    /**
     * Adds a new handler for the given constraint type.
     */
    void add_handler(ConstraintType type, HFunc handler);

    /**
     * Removes the handler for the given constraint type.
     */
     void remove_handler(ConstraintType type);

    /**
     * Handles the given constraint by the appropriate handler function and returns
     * @param constraint
     * @return
     */
    HFuncRet handle(HThis* instance, Constraint constraint, HFuncArgs... args);
};

#include "ConstraintHandler.ipp"
