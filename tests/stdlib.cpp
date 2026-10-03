#include <Utils/Text.h>
#include <Utils/Sequence.h>

#include <cassert>
#include <cstring>
#include <stdexcept>

int main() {
    assert(formatText("%s:%04d:%c:%.2f", "name", 7, 'X', 1.25) == "name:0007:X:1.25");
    std::string longText(16384, 'x');
    assert(formatText("[%s] %d %s", longText.c_str(), 42, "tail") == "[" + longText + "] 42 tail");
    assert(formatText("%c%c%c", 'a', 0, 'b') == std::string("a\0b", 3));
    assert(formatText("").empty());
    assert(compareAsciiCaseInsensitive("SoundZ", "soundz") == 0);
    assert(compareAsciiCaseInsensitive("abc", "ABCD") < 0);
    assert(compareAsciiCaseInsensitive("ABD", "abc") > 0);
    assert(asciiUpper(static_cast<char>(0xe9)) == static_cast<char>(0xe9));
    assert(asciiLower('Z') == 'z');
    assert(stringIndex(longText.find('z')) == -1);
    assert(stringIndex(longText.find('x')) == 0);
    assert(sliceText("abc", -4, 2) == "ab");
    assert(sliceText("abc", 8).empty());
    assert(sliceText("abc", 1, -1).empty());
    assert(rightText("abc", 8) == "abc");
    assert(rightText("abc", -1).empty());

    std::vector<std::string> values;
    values.push_back(longText);
    const int distantIndex = static_cast<int>(values.capacity()) + 20;
    growAndAssign(values, distantIndex, values[0]);
    assert(values[0] == longText && values[distantIndex] == longText);
    assert(values[1].empty());
    bool rejected = false;
    try { growAndAssign(values, -1, longText); }
    catch (const std::out_of_range&) { rejected = true; }
    assert(rejected);

    std::list<std::string> queue;
    queue.push_back("first");
    queue.push_back("middle");
    queue.push_back(longText);
    assert(iteratorAt(queue.begin(), queue.end(), -1) == queue.end());
    assert(iteratorAt(queue.begin(), queue.end(), 3) == queue.end());
    std::list<std::string>::iterator middle = iteratorAt(queue.begin(), queue.end(), 1);
    assert(takeFront(queue) == "first");
    assert(*middle == "middle");
    assert(takeBack(queue) == longText);
    assert(takeFront(queue) == "middle");
    assert(queue.empty());
}
