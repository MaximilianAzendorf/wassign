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

#include "ConstraintHandler.h"

template<typename HFuncRet, typename HThis, typename... HFuncArgs>
ConstraintHandler<HFuncRet, HThis, HFuncArgs...>::ConstraintHandler(
        std::initializer_list<pair<ConstraintType, HFunc>> initializerList)
    : _handlers(initializerList)
{
}

template<typename HFuncRet, typename HThis, typename... HFuncArgs>
void ConstraintHandler<HFuncRet, HThis, HFuncArgs...>::add_handler(ConstraintType type, ConstraintHandler::HFunc handler)
{
    bool success = _handlers.insert(std::make_pair(type, handler)).second;
    if(!success)
    {
        throw std::logic_error("Handler for constraint type " + Constraint::type_name(type) + " already exists.");
    }
}

template<typename HFuncRet, typename HThis, typename... HFuncArgs>
void ConstraintHandler<HFuncRet, HThis, HFuncArgs...>::remove_handler(ConstraintType type)
{
    auto handlerIt = find_handler(type);
    _handlers.erase(handlerIt);
}

template<typename HFuncRet, typename HThis, typename... HFuncArgs>
HFuncRet ConstraintHandler<HFuncRet, HThis, HFuncArgs...>::handle(HThis* instance, Constraint constraint, HFuncArgs... args)
{
    auto handlerIt = find_handler(constraint.type());
    return (instance->*handlerIt)(constraint, args...);
}

template<typename HFuncRet, typename HThis, typename... HFuncArgs>
typename ConstraintHandler<HFuncRet, HThis, HFuncArgs...>::HFunc*
ConstraintHandler<HFuncRet, HThis, HFuncArgs...>::find_handler(ConstraintType type)
{
    auto handlerIt = _handlers.find(type);

    if(handlerIt == _handlers.end())
    {
        throw std::logic_error("No handler for constraint type " + Constraint::type_name(type) + ".");
    }

    return handlerIt;
}
