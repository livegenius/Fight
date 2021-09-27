#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <string>
#include <vector>

#undef DrawText //Windows namespace pollution
int DrawText(std::string text, std::vector<float> &arr, float x, float y); 


#endif // UTIL_H_INCLUDED
