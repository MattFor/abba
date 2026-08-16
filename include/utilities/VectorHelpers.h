//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_VECTORHELPERS_H
#define ABBA_VECTORHELPERS_H

#include <vector>
#include <algorithm>

template <typename T>
bool vector_has(const std::vector<T>& vector, const T& element)
{
    return std::find(vector.begin(), vector.end(), element) != vector.end();
}

#endif //ABBA_VECTORHELPERS_H
