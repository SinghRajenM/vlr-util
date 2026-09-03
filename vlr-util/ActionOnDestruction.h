#pragma once

#include <functional>
#include <optional>

#include "config.h"

#include "BaseWithVirtualDestructor.h"

namespace vlr {

template< typename TActionResult >
class CActionOnDestruction
	: public CBaseWithVirtualDestructor
{
	static constexpr bool ActionHasValidResult = !std::is_same_v<TActionResult, void>;

public:
	using FAction = std::function<TActionResult()>;

public:
	FAction m_fAction;

protected:
	decltype(auto) DoActionWithPossibleResult(const FAction& fAction)
	{
		if (!fAction)
		{
			if constexpr (ActionHasValidResult)
			{
				return TActionResult{};
			}
			else
			{
				return;
			}
		}
		return fAction();
	}

public:
	decltype(auto) DoAction()
	{
		return DoActionWithPossibleResult(m_fAction);
	}
	decltype(auto) DoActionAndClear()
	{
		// Note: Using a "manual" version of this class, to avoid copying m_fAction
		// (we cannot "capture" the result to return, because it might be void)
		auto fClearAction = [this](void*) { m_fAction = {}; };
		// Note: Pointer must not be nullptr, so destructor is called
		auto oOnDelete_ClearAction = std::unique_ptr<void, decltype(fClearAction)>{ (void*)0x42, fClearAction };
		return DoActionWithPossibleResult(m_fAction);
	}
	void ClearAction()
	{
		m_fAction = {};
	}

public:
	CActionOnDestruction() = default;
	CActionOnDestruction(const FAction& fAction)
		: m_fAction{ fAction }
	{
	}
	CActionOnDestruction(FAction&& fAction)
		: m_fAction{ std::move(fAction) }
	{
	}
	// Note: We allow move, but not copy, since we do not currently do reference counting
	CActionOnDestruction(CActionOnDestruction&&) = default;
	CActionOnDestruction(const CActionOnDestruction&) = delete;
	virtual ~CActionOnDestruction()
	{
		DoActionAndClear();
	}
};

template <typename TFunctor>
class CEfficientActionOnDestruction
{
public:
	using TActionResult = std::invoke_result_t<TFunctor>;

protected:
	static constexpr bool ActionHasValidResult = !std::is_same_v<TActionResult, void>;
	TFunctor m_fAction;
	bool m_bCallAction = true;

public:
	explicit CEfficientActionOnDestruction(const TFunctor& fAction)
		: m_fAction{ fAction }
	{}
	explicit CEfficientActionOnDestruction(TFunctor&& fAction)
		: m_fAction{ std::forward<TFunctor>(fAction) }
	{}
	// Allow move while disabling the other call, as necessary
	CEfficientActionOnDestruction(CEfficientActionOnDestruction&& oOther)
		noexcept(std::is_nothrow_move_constructible_v<TFunctor>)
		: m_fAction{ std::move(oOther.m_fAction) }
		, m_bCallAction{ oOther.m_bCallAction }
	{
		oOther.m_bCallAction = false;
	}
	CEfficientActionOnDestruction(const CEfficientActionOnDestruction&) = delete;
	CEfficientActionOnDestruction& operator=(const CEfficientActionOnDestruction&) = delete;
	CEfficientActionOnDestruction& operator=(CEfficientActionOnDestruction&& oOther)
		noexcept(std::is_nothrow_move_assignable_v<TFunctor>)
	{
		if (this != &oOther)
		{
			m_fAction = std::move(oOther.m_fAction);
			m_bCallAction = oOther.m_bCallAction;
			oOther.m_bCallAction = false;
		}
		return *this;
	}
	~CEfficientActionOnDestruction()
	{
		if (m_bCallAction)
		{
			DoAction();
		}
	}

	decltype(auto) DoAction()
	{
		if (!m_bCallAction)
		{
			if constexpr (ActionHasValidResult)
			{
				return TActionResult{};
			}
			else
			{
				return;
			}
		}

		return m_fAction();
	}
	decltype(auto) DoActionAndClear()
	{
		if (!m_bCallAction)
		{
			if constexpr (ActionHasValidResult)
			{
				return TActionResult{};
			}
			else
			{
				return;
			}
		}

		m_bCallAction = false;
		return m_fAction();
	}
	inline void ClearAction()
	{
		m_bCallAction = false;
	}

};

template <typename TFunctor>
inline auto MakeActionOnDestruction(TFunctor&& fAction)
{
	//return CActionOnDestruction<decltype(std::declval<TFunctor>()())>{ std::forward<TFunctor>(fAction) };
	return CEfficientActionOnDestruction<std::decay_t<TFunctor>>{ std::forward<TFunctor>(fAction) };
}

} // namespace vlr
