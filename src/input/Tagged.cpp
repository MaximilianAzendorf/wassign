#include "Tagged.h"
#include <boost/algorithm/string.hpp>
#include <magic_enum.hpp>

using namespace magic_enum;

map<Tag, string> Tagged::_tagNames = map<Tag, string>();

Tagged::Tagged(Tag tag, int value)
    : _tag(tag),
    _values({value})
{
}

Tagged::Tagged(Tag tag, vector<int> values)
    : _tag(tag),
    _values(std::move(values))
{

}

Tag Tagged::tag() const
{
    return _tag;
}

vector<int> Tagged::values() const
{
    return _values;
}

string Tagged::tag_name(Tag tag)
{
    if(_tagNames.empty())
    {
        for(auto entry : enum_entries<Tag>())
        {
            _tagNames[entry.first] = boost::algorithm::to_lower_copy(string{entry.second});
        }
    }

    return _tagNames.at(tag);
}

int Tagged::value(int index) const
{
    return _values[index];
}
