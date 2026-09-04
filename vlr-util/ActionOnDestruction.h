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
		try
		{
			// Note: We do not need to clear here, because we're in the destructor, and the object is being destroyed anyway
			DoAction();
		}
		catch (...)
		{
			// Note: We cannot safely throw exceptions from this destructor. If the action throws, we will terminate the process.
			// So we will just swallow the exception and continue.
		}
	}
};

template <typename TFunctor>
class CEfficientActionOnDestruction
{
public:
	using TActionResult = std::invoke_result_t<TFunctor>;

protected:
	static constexpr bool m_bActionHasValidResult = !std::is_same_v<TActionResult, void>;
	static constexpr bool m_bDefaultResultProductionIsNoexcept = (!m_bActionHasValidResult) || noexcept(TActionResult());
	static constexpr bool m_bActionIsNoexcept = std::is_nothrow_invocable_v<TFunctor>;
	static constexpr bool m_bDoActionIsNoexcept = m_bActionIsNoexcept && m_bDefaultResultProductionIsNoexcept;

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
		if (!m_bCallAction)
		{
			return;
		}

		// Note: We do not need to clear here, because we're in the destructor, and the object is being destroyed anyway

		if constexpr (m_bActionIsNoexcept)
		{
			DoAction();
		}
		else
		{
			try
			{
				DoAction();
			}
			catch (...)
			{
				// Note: We cannot safely throw exceptions from this destructor. If the action throws, we will terminate the process.
				// So we will just swallow the exception and continue.
			}
		}
	}

	// Note: We allow explicit calls to DoAction() and DoActionAndClear() to throw exceptions from the underlying action, 
	// so that the caller can handle them, if desired. But we cannot allow exceptions to propagate from the destructor.
	// Note: Using static constexpr members in noexcept is legal, as long as the declaration is visible at the point of use. 
	// So we can use them in noexcept here.

	decltype(auto) ProduceDefaultResultOnNoAction() noexcept(m_bDefaultResultProductionIsNoexcept)
	{
		if constexpr (m_bActionHasValidResult)
		{
			return TActionResult{};
		}
		else
		{
			return;
		}
	}
	decltype(auto) DoAction() noexcept(m_bDoActionIsNoexcept)
	{
		if (!m_bCallAction)
		{
			return ProduceDefaultResultOnNoAction();
		}

		return m_fAction();
	}
	decltype(auto) DoActionAndClear() noexcept(m_bDoActionIsNoexcept)
	{
		if (!m_bCallAction)
		{
			return ProduceDefaultResultOnNoAction();
		}

		m_bCallAction = false;
		return m_fAction();
	}
	inline void ClearAction() noexcept
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
