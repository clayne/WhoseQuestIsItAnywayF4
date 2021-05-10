#include "Hooks.h"

namespace
{
	[[nodiscard]] auto GetOriginalDataList(RE::BSTSmartPointer<RE::ExtraDataList> a_list)
		-> RE::BSTSmartPointer<RE::ExtraDataList>
	{
		const auto xHandles = a_list ? a_list->GetByType<RE::ExtraReferenceHandles>() : nullptr;
		const auto original = xHandles ? xHandles->originalRef.get() : nullptr;
		return original && original->extraList ? original->extraList : a_list;
	}

	[[nodiscard]] RE::TESQuest* GetQuestFromItem(
		const RE::BGSInventoryItem& a_item,
		std::uint32_t a_stackID)
	{
		const RE::BSTSmartPointer stack{ a_item.GetStackByID(a_stackID) };
		const auto xlist = stack ? GetOriginalDataList(stack->extra) : nullptr;
		const auto xalias = xlist ? xlist->GetByType<RE::ExtraAliasInstanceArray>() : nullptr;
		if (xalias) {
			const RE::BSAutoReadLock l{ xalias->aliasArrayLock };
			for (const auto& alias : xalias->aliasArray) {
				if (alias.alias && alias.alias->IsQuestObject()) {
					return alias.quest;
				}
			}
		}

		return nullptr;
	}

	void ShowHUDMessage(
		const RE::BGSInventoryItem* a_item,
		std::uint32_t a_stackID)
	{
		const auto quest = a_item ? GetQuestFromItem(*a_item, a_stackID) : nullptr;
		const auto message = [&]() -> std::string {
			if (quest) {
				auto result = fmt::format(
					FMT_STRING("Quest item locked by: [{:08X}]"),
					quest->formID);
				const auto id =
					!quest->fullName.empty() ?
						static_cast<std::string_view>(quest->fullName) :
                        static_cast<std::string_view>(quest->formEditorID);
				if (!id.empty()) {
					result += fmt::format(FMT_STRING(" {}"), id);
				}
				result += '.';

				return result;
			} else {
				const auto gmst = RE::GameSettingCollection::GetSingleton();
				const auto iter = gmst ? gmst->settings.find("sDropQuestItemWarning") : gmst->settings.end();
				return iter != gmst->settings.end() ? std::string(iter->second->GetString()) : ""s;
			}
		}();

		RE::SendHUDMessage::ShowHUDMessage(message.c_str(), nullptr, true, true);
	}

	void DropItemHook(
		std::uint32_t a_handleID,
		std::uint32_t a_stackID)
	{
		const auto inv = RE::BGSInventoryInterface::GetSingleton();
		const auto item = inv ? inv->RequestInventoryItem(a_handleID) : nullptr;
		ShowHUDMessage(item, a_stackID);
	}

	void TransferItemHook(
		const RE::BGSInventoryItem* a_item,
		const RE::BSTSmallArray<std::uint16_t, 4>& a_arr)
	{
		ShowHUDMessage(a_item, a_arr.front());
	}
}

#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <xbyak/xbyak.h>

namespace
{
	struct DropPatch :
		Xbyak::CodeGenerator
	{
		DropPatch()
		{
			mov(ecx, ptr[rsp + 0x8 + 0x88 + 0x8]);

			mov(rdx, ptr[rbx]);
			mov(edx, ptr[rdx]);

			mov(rax, reinterpret_cast<std::uintptr_t>(DropItemHook));
			jmp(rax);
		}
	};

	struct TransferPatch :
		Xbyak::CodeGenerator
	{
		TransferPatch()
		{
			mov(rcx, ptr[rsp + 0x8 + 0x210 - 0x1B8]);

			mov(rdx, ptr[rbp + 0x110 + 0x18]);
			lea(rdx, ptr[rdx + 0x8]);

			mov(rax, reinterpret_cast<std::uintptr_t>(TransferItemHook));
			jmp(rax);
		}
	};

	template <class T>
	void WritePatch(REL::ID a_id, std::size_t a_start, std::size_t a_end)
	{
		const auto size = a_end - a_start;
		assert(size >= 6);

		REL::Relocation<std::uintptr_t> target{ a_id };
		REL::safe_fill(target.address() + a_start, REL::NOP, size);

		T p;
		p.ready();

		auto& trampoline = F4SE::GetTrampoline();
		trampoline.write_call<6>(
			target.address() + a_start,
			trampoline.allocate(p));
	}
}

namespace Hooks
{
	void Install()
	{
		WritePatch<DropPatch>(REL::ID(311743), 0x6E, 0x83);
		WritePatch<TransferPatch>(REL::ID(552046), 0x922, 0x937);

		logger::debug("installed all hooks"sv);
	}
}
