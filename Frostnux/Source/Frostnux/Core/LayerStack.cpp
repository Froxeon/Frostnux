#include <fxpch.h>
#include "LayerStack.h"

namespace Frostnux {

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
			delete layer;
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.insert(m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsertIndex), layer);
		++m_LayerInsertIndex;
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.push_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it == m_Layers.end())
			return;

		const std::size_t idx = static_cast<std::size_t>(it - m_Layers.begin());
		m_Layers.erase(it);

		if (idx < m_LayerInsertIndex)
			--m_LayerInsertIndex;
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
			m_Layers.erase(it);
	}

	void LayerStack::Clear()
	{
		for (Layer* layer : m_Layers)
			delete layer;
		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}

}
