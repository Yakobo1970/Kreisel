#pragma once
#include "Model.h"

namespace kr
{
int numFactoryPresets();
juce::String factoryCategory (int index);
juce::String factoryName (int index);
Snapshot makeFactoryPreset (int index);
juce::StringArray factoryCategories();
Snapshot defaultSnapshot();
juce::String validateFactoryPresets(); // empty = every library spec parses, names unique
} // namespace kr
