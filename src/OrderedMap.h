#pragma once
#include <vector>

#include "RepoOrchestrator.h"

template<typename Key, typename Value>
class OrderedMap
{
public:
	OrderedMap() = default;

	auto begin() const
	{
		return data.begin();
	}

	auto end() const
	{
		return data.end();
	}

	auto Find(const Key& key) 
	{
		auto it = data.begin();
		for (; it != data.end(); ++it)
			if (it->first == key)
				break;
		return it;
	}

	size_t size() const
	{
		return data.size();
	}

	void Emplace(const Key& key, const Value& value)
	{
		auto it = Find(key);
		if(it != end())
		{
			it->second = value;
		}
		else
		{
			data.push_back(std::make_pair<>(key, value));
		}
	}

	void Clear()
	{
		data.clear();
	}

	Value& operator[](const Key& index)
	{
		auto it = Find(index);
		if (it != end())
			return it->second;

		return data.emplace_back(index, Value{}).second;
	}

private:
	std::vector<std::pair<const Key, Value>> data;
};
