/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef WIN32
	// VS6 doesn't like the length of STL generated names: disabling
	#pragma warning(disable:4786)
	// Quagmire: Dont know why the one in debug.h doesnt work, but it doesnt.
#endif
#include "../common/debug.h"
#ifdef _CRTDBG_MAP_ALLOC
	#undef new
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#include <sstream>
#include <iostream>
#include "../common/Item.h"
#include "../common/misc.h"
#include "../common/races.h"
using namespace std;

// Create appropriate ItemInst class
#ifndef PACKETCOLLECTOR
ItemInst* ItemInst::Create(uint32 item_id, sint16 charges, uint32 aug1, uint32 aug2, uint32 aug3, uint32 aug4, uint32 aug5)
{
	const Item_Struct* item = NULL;
	item = database.GetItem(item_id);
	return ItemInst::Create(item, charges, aug1, aug2, aug3, aug4, aug5);
}
#endif
// Create appropriate ItemInst class
ItemInst* ItemInst::Create(const Item_Struct* item, sint16 charges, uint32 aug1, uint32 aug2, uint32 aug3, uint32 aug4, uint32 aug5)
{
	ItemInst* inst = NULL;
	if (item) {
		if (charges == 0)
			charges = item->Common.MaxCharges;
		switch (item->ItemClass) {
		case ItemTypeCommon:
			inst = new ItemCommonInst(item, charges, aug1, aug2, aug3, aug4, aug5);
			break;
		case ItemTypeContainer:
			inst = new ItemContainerInst(item, charges);
			break;
		case ItemTypeBook:
			inst = new ItemBookInst(item, charges);
			break;
		}
		inst->SetCharges(charges);
	}
	
	return inst;
}

ItemCommonInst::ItemCommonInst(const Item_Struct* item , sint16 charges , uint32 aug1 , uint32 aug2 , uint32 aug3 , uint32 aug4 , uint32 aug5 ) : ItemInst(item, charges)
{
	PutAugment(0,aug1);
	PutAugment(1,aug2);
	PutAugment(2,aug3);
	PutAugment(3,aug4);
	PutAugment(4,aug5);
}

ItemCommonInst::ItemCommonInst(uint32 item_id, sint16 charges , uint32 aug1 , uint32 aug2 , uint32 aug3 , uint32 aug4 , uint32 aug5 ) : ItemInst(item_id, charges) 
{
	PutAugment(0,aug1);
	PutAugment(1,aug2);
	PutAugment(2,aug3);
	PutAugment(3,aug4);
	PutAugment(4,aug5);
}
		 
// Make a copy of an ItemCommonInst object
ItemCommonInst::ItemCommonInst(const ItemCommonInst& copy) : ItemInst((ItemInst&)copy)
{
	// Copy augments
	iter_augment it;
	for (it=copy.m_augments.begin(); it!=copy.m_augments.end(); it++) {
		ItemCommonInst* augment_old = it->second;
		ItemCommonInst* augment_new = new ItemCommonInst(augment_old->m_item, augment_old->m_charges);
		m_augments[it->first] = augment_new;
	}
}

// Clean up augmented sub objects
ItemCommonInst::~ItemCommonInst()
{
	// Destroy augments
	iter_augment it;
	for (it=m_augments.begin(); it!=m_augments.end(); it++) {
		ItemCommonInst* augment = it->second;
		if (augment != NULL)
			safe_delete(augment);
	}
}

// Make a copy of an ItemContainerInst object
ItemContainerInst::ItemContainerInst(const ItemContainerInst& copy) : ItemInst((ItemInst&)copy)
{
	// Copy container contents
	iter_bag it;
	for (it=copy.m_contents.begin(); it!=copy.m_contents.end(); it++) {
		ItemInst* inst_old = it->second;
		ItemInst* inst_new = NULL;
		
		if (inst_old) {
			inst_new = inst_old->Clone();
		}
		
		if (inst_new != NULL) {
			m_contents[it->first] = inst_new;
		}
	}
}

// Clean up container contents
ItemContainerInst::~ItemContainerInst()
{
	Clear();
}

// Clone a type of ItemInst object
// c++ doesn't allow a polymorphic copy constructor,
// so we have to resort to a polymorphic Clone()
ItemInst* ItemCommonInst::Clone() const
{
	// Pseudo-polymorphic copy constructor
	return new ItemCommonInst(*this);
}

// Clone a type of ItemInst object
// c++ doesn't allow a polymorphic copy constructor,
// so we have to resort to a polymorphic Clone()
ItemInst* ItemContainerInst::Clone() const
{
	// Pseudo-polymorphic copy constructor
	return new ItemContainerInst(*this);
}

// Clone a type of ItemInst object
// c++ doesn't allow a polymorphic copy constructor,
// so we have to resort to a polymorphic Clone()
ItemInst* ItemBookInst::Clone() const
{
	// Pseudo-polymorphic copy constructor
	return new ItemBookInst(*this);
}

// Query item type
bool ItemInst::IsType(ItemType item_type) const
{
	if (!m_item)
		return false;
	
	return (m_item->ItemClass == item_type);
}

// Query item type
// Overrides base class implementation
bool ItemContainerInst::IsType(ItemType item_type) const
{
	// Check usage type
	if ((m_use_type == ItemUseWorldContainer) && (item_type == ItemTypeContainer))
		return true;
	
	// Delegate to base class
	return ItemInst::IsType(item_type);
}

// Query Attribute of item
bool ItemInst::IsAttrib(ItemAttrib attribs) const
{
	if (!m_item)
		return false;
	
	return (m_item->attribs & attribs);
}

// Can item be stacked?
bool ItemInst::IsStackable() const
{
	// Base class; sub classes must override to say otherwise
	return false;
}

// Can item be equipped by?
bool ItemInst::IsEquipable(int16, int16) const
{
	// Base class; sub classes must override to say otherwise
	return false;
}

// Can item be equipped at?
bool ItemInst::IsEquipable(sint16) const
{
	// Base class; sub classes must override to say otherwise
	return false;
}

// Has attack/delay?
bool ItemInst::IsWeapon() const
{
	// Base class; sub classes must override to say otherwise
		return false;
}

// Is item stackable?
bool ItemCommonInst::IsStackable() const
{
	//This function is not correct. Not all stackable items have itemuse 
	//set to ItemUseStackable, example: fishing grubs
	if (m_item)
		return ((m_item->Common.MaxCharges == 1) && (m_item->Common.ItemUse>=14) && (m_item->Common.ItemUse<=19));
	
	return false;
}

// Can item be equipped?

bool ItemCommonInst::IsEquipable(int16 race, int16 class_) const
{
	if (!m_item)
		return false;
	
	bool israce = false;
	bool isclass = false;
	
	if (m_item->EquipSlots == 0) {
		return false;
	}
	
	uint32 classes_ = m_item->Common.Classes;
	uint32 races_ = m_item->Common.Races;
	int32 race_ = 0;
	#ifndef PACKETCOLLECTOR
	race_ = GetArrayRace(race);
	#endif
	// @merth: can this be optimized?  i.e., will (race & common->Races) suffice?
	for (int cur_class = 1; cur_class<=15; cur_class++) {
		if (classes_ % 2 == 1) {
    		if (cur_class == class_) {
    			isclass = true;
			}
		}
		classes_ >>= 1;
	}
	for (unsigned int cur_race = 1; cur_race <= 15; cur_race++) {
		
		if (races_ % 2 == 1) {
    		if (cur_race == race_) {
    			israce = true;
   			}
		}
		races_ >>= 1;
	}
	
	return (israce && isclass);
}

// Can equip at this slot?
bool ItemCommonInst::IsEquipable(sint16 slot_id) const
{
	if (!m_item)
		return false;
	
	if (slot_id < 22) {
		uint32 slot_mask = (1 << slot_id);
		if (slot_mask & m_item->EquipSlots)
			return true;
	}
	
	return false;
}

sint8 ItemCommonInst::AvailableAugmentSlot(sint32 augtype) const
{
	if (!m_item)
		return -1;

	int i;
	for (i=0;i<5;i++) {
		if (!GetAugment(i)) {
			if (augtype==-1 || (m_item->Common.AugSlotType[i] && (1<<(m_item->Common.AugSlotType[i]-1) & augtype)))
				break;
		}

	}

	return (i<5) ? i : -1;
}
uint32 ItemCommonInst::GetAugmentItemID(uint8 slot) const
{
const ItemCommonInst *aug;
uint32 id=0;
	if ((aug=GetAugment(slot))!=NULL)
		id= aug->GetItem()->ItemNumber;

	return id;
}


// Has attack/delay?
bool ItemCommonInst::IsWeapon() const
{
	if (!m_item)
		return false;
	if(m_item->Common.ItemUse==ItemUseArrow && m_item->Common.Damage != 0)
		return true;
	else
		return ((m_item->Common.Damage != 0) && (m_item->Common.Delay != 0));
}

// Retrieve augment inside item
ItemCommonInst* ItemCommonInst::GetAugment(uint8 slot) const
{
	iter_augment it = m_augments.find(slot);
	if (it != m_augments.end()) {
		return it->second;
	}
	
	return NULL;
}

// Remove augment from item and destroy it
void ItemCommonInst::DeleteAugment(uint8 index)
{
	iter_augment it = m_augments.find(index);
	if (it != m_augments.end()) {
		ItemCommonInst* augment = it->second;
		m_augments.erase(index);
		safe_delete(augment);
	}
}

// Remove augment from item and return it
ItemCommonInst* ItemCommonInst::RemoveAugment(uint8 index)
{
	iter_augment it = m_augments.find(index);
	if (it != m_augments.end()) {
		ItemCommonInst* augment = it->second;
		m_augments.erase(index);
		return augment;
	}

	return NULL;
}

// Add an augment to the item
void ItemCommonInst::PutAugment(uint8 slot, const ItemCommonInst& augment)
{
	// Remove item currently in slot (if any)
	ItemCommonInst* current = GetAugment(slot);
	safe_delete(current);
	
	// Replace ptr in map held by former augment with ours
	_PutAugment(slot, (ItemCommonInst*)augment.Clone());
}

void ItemCommonInst::PutAugment(uint8 slot, uint32 item_id)
{
	if (item_id!=0) {
	        const ItemCommonInst aug(item_id);
		PutAugment(slot,aug);
	}
}

// Retrieve item inside container
ItemInst* ItemContainerInst::GetItem(uint8 index) const
{
	iter_bag it = m_contents.find(index);
	if (it != m_contents.end()) {
		ItemInst* inst = it->second;
		return inst;
	}
	
	return NULL;
}

void ItemContainerInst::PutItem(uint8 index, const ItemInst& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(index);
	
	if (!inst) {
		// User must be effectively deleting the item
		// in the slot, why hold a null ptr in map<>?
		return;
	}

//	if (index<0 || index>9) {
//		LogFile->write(EQEMuLog::Error, "ItemContainerInst::PutItem: Invalid index specified (%i)", index);
//		return;
//	}
	
	// Delegate to internal method
	_PutItem(index, inst.Clone());
}

// Remove item inside container
void ItemContainerInst::DeleteItem(uint8 index)
{
	ItemInst* inst = PopItem(index);
	safe_delete(inst);
}

// Remove all items from container
void ItemContainerInst::Clear()
{
	// Destroy container contents
	//iter_bag it;
	//for (it=m_contents.begin(); it!=m_contents.end(); it++) {
		//ItemInst* inst = it->second;
		//safe_delete(inst);

	//}
	for(int i=0;i<10;i++)
		m_contents.erase(i);
}

// Remove item from container without memory delete
// Hands over memory ownership to client of this function call
ItemInst* ItemContainerInst::PopItem(uint8 index)
{
	iter_bag it = m_contents.find(index);
	if (it != m_contents.end()) {
		ItemInst* inst = it->second;
		m_contents.erase(index);
		return inst;
	}
	
	// Return pointer that needs to be deleted (or otherwise managed)
	return NULL;
}

// Put item onto back of queue
void ItemInstQueue::push(ItemInst* inst)
{
	m_list.push_back(inst);
}

// Put item onto front of queue
void ItemInstQueue::push_front(ItemInst* inst)
{
	m_list.push_front(inst);
}

// Remove item from front of queue
ItemInst* ItemInstQueue::pop()
{
	if (m_list.size() == 0)
		return NULL;
	
	ItemInst* inst = m_list.front();
	m_list.pop_front();
	return inst;
}

// Look at item at front of queue
ItemInst* ItemInstQueue::peek_front() const
{
	return (m_list.size()==0) ? NULL : m_list.front();
}

// Retrieve item at specified slot; returns false if item not found
ItemInst* Inventory::GetItem(sint16 slot_id) const
{
	_CP(Inventory_GetItem);
	ItemInst* result = NULL;
	
	// Cursor
	if (slot_id == SLOT_CURSOR) {
		// Cursor slot
		result = m_cursor.peek_front();
	}
	
	// Non bag slots
	else if (slot_id>=3000 && slot_id<=3007) {
		// Trade slots
		result = _GetItem(m_trade, slot_id);
	}
	else if (slot_id>=2500 && slot_id<=2501) {
		// Shared Bank slots
		result = _GetItem(m_shbank, slot_id);
	}
	else if (slot_id>=2000 && slot_id<=2015) {
		// Bank slots
		result = _GetItem(m_bank, slot_id);
	}
	else if ((slot_id>=22 && slot_id<=29)) {
		// Personal inventory slots
		result = _GetItem(m_inv, slot_id);
	}
	else if ((slot_id>=0 && slot_id<=21) || (slot_id >= 400 && slot_id<=404)) {
		// Equippable slots (on body)
		result = _GetItem(m_worn, slot_id);
	}
	
	// Inner bag slots
	else if (slot_id>=3031 && slot_id<=3110) {
		// Trade bag slots
		ItemInst* inst = _GetItem(m_trade, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			result = bag->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=2531 && slot_id<=2550) {
		// Shared Bank bag slots
		ItemInst* inst = _GetItem(m_shbank, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			result = bag->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=2031 && slot_id<=2190) {
		// Bank bag slots
		ItemInst* inst = _GetItem(m_bank, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			result = bag->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=331 && slot_id<=340) {
		// Cursor bag slots
		ItemInst* inst = m_cursor.peek_front();
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			result = bag->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=251 && slot_id<=330) {
		// Personal inventory bag slots
		ItemInst* inst = _GetItem(m_inv, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			result = bag->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	
	return result;
}

// Retrieve item at specified position within bag
ItemInst* Inventory::GetItem(sint16 slot_id, uint8 bagidx) const
{
	return GetItem(Inventory::CalcSlotId(slot_id, bagidx));
}

sint16 Inventory::PushCursor(const ItemInst& inst)
{
	m_cursor.push(inst.Clone());
	return SLOT_CURSOR;
}

// Put an item snto specified slot
sint16 Inventory::PutItem(sint16 slot_id, const ItemInst& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(slot_id);
	
	if (!inst) {
		// User is effectively deleting the item
		// in the slot, why hold a null ptr in map<>?
		return slot_id;
	}
	
	// Delegate to internal method
	return _PutItem(slot_id, inst.Clone());
}

// Swap items in inventory
void Inventory::SwapItem(sint16 slot_a, sint16 slot_b)
{
	// Temp holding area for a
	ItemInst* inst_a = GetItem(slot_a);
	
	// Copy b->a
	_PutItem(slot_a, GetItem(slot_b));
	
	// Copy a->b
	_PutItem(slot_b, inst_a);
}
#ifndef PACKETCOLLECTOR

// Checks that user has at least 'quantity' number of items in a given inventory slot
// Returns first slot it was found in, or SLOT_INVALID if not found

//This function has a flaw in that it only returns the last stack that it looked at
//when quantity is greater than 1 and not all of quantity can be found in 1 stack.

sint16 Inventory::HasItem(uint32 item_id, uint8 quantity, uint8 where)
{
	_CP(Inventory_HasItem);
	const Item_Struct* item = database.GetItem(item_id);
	sint16 slot_id = SLOT_INVALID;
	
	//Altered by Father Nitwit to support a specification of
	//where to search, with a default value to maintain compatibility
	
	// Check each inventory bucket
	if(where & invWhereWorn) {
		slot_id = _HasItem(m_worn, item, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	if(where & invWherePersonal) {
		slot_id = _HasItem(m_inv, item, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereBank) {
		slot_id = _HasItem(m_bank, item, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereSharedBank) {
		slot_id = _HasItem(m_shbank, item, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereTrading) {
		slot_id = _HasItem(m_trade, item, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItem(m_cursor, item, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	return slot_id;
}

//this function has the same quantity flaw mentioned above in HasItem()

sint16 Inventory::HasItemByUse(uint8 use, uint8 quantity, uint8 where)
{
	sint16 slot_id = SLOT_INVALID;
	
	// Check each inventory bucket
	if(where & invWhereWorn) {
		slot_id = _HasItemByUse(m_worn, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	if(where & invWherePersonal) {
		slot_id = _HasItemByUse(m_inv, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereBank) {
		slot_id = _HasItemByUse(m_bank, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereSharedBank) {
		slot_id = _HasItemByUse(m_shbank, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereTrading) {
		slot_id = _HasItemByUse(m_trade, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItemByUse(m_cursor, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	return slot_id;
}
#endif

// Remove item from inventory (with memory delete)
void Inventory::DeleteItem(sint16 slot_id, uint8 quantity)
{
	// Pop item out of inventory map (or queue)
	ItemInst* item_to_delete = PopItem(slot_id);
	
	// Determine if object should be fully deleted, or
	// just a quantity of charges of the item can be deleted
	if (item_to_delete && (quantity > 0)) {
		item_to_delete->SetCharges(item_to_delete->GetCharges() - quantity);
		if (item_to_delete->GetCharges() > 0) {
			// Charges still exist!  Put back into inventory
			_PutItem(slot_id, item_to_delete);
			return;
		}
	}
	
	// Item can now be destroyed
	safe_delete(item_to_delete);
}

// Remove item from bucket without memory delete
// Returns item pointer if full delete was successful
ItemInst* Inventory::PopItem(sint16 slot_id)
{
	ItemInst* p = NULL;
	
	if (slot_id==SLOT_CURSOR) { // Cursor
		p = m_cursor.pop();
	}
	else if ((slot_id>=0 && slot_id<=21) || (slot_id >= 400 && slot_id<=404)) { // Worn slots
		p = m_worn[slot_id];
		m_worn.erase(slot_id);
	}
	else if ((slot_id>=22 && slot_id<=29)) {
		p = m_inv[slot_id];
		m_inv.erase(slot_id);
	}
	else if (slot_id>=2000 && slot_id<=2015) { // Bank slots
		p = m_bank[slot_id];
		m_bank.erase(slot_id);
	}
	else if (slot_id>=2500 && slot_id<=2501) { // Shared bank slots
		p = m_shbank[slot_id];
		m_shbank.erase(slot_id);
	}
	else if (slot_id>=3000 && slot_id<=3007) { // Trade window slots
		p = m_trade[slot_id];
		m_trade.erase(slot_id);
	}
	else {
		// Is slot inside bag?
		ItemInst* baginst = GetItem(Inventory::CalcSlotId(slot_id));
		if (baginst != NULL && baginst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)baginst;
			p = bag->PopItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	
	// Return pointer that needs to be deleted (or otherwise managed)
	return p;
}

// Locate an available inventory slot
// Returns slot_id when there's one available, else SLOT_INVALID
sint16 Inventory::FindFreeSlot(bool for_bag, bool try_cursor, int8 min_size)
{
	// Check basic inventory
	for (sint16 i=22; i<=29; i++) {
		if (!GetItem(i))
			// Found available slot in personal inventory
			return i;
	}
	
	if (!for_bag) {
		for (sint16 i=22; i<=29; i++) {
			const ItemInst* inst = GetItem(i);
			if (inst && inst->IsType(ItemTypeContainer) 
				&& inst->GetItem()->Container.SizeCapacity >= min_size
				) {
				sint16 base_slot_id = Inventory::CalcSlotId(i, 0);

				int8 slots=inst->GetItem()->Container.Slots;
				uint8 j;
				for (j=0; j<slots; j++) {
					if (!GetItem(base_slot_id + j))
						// Found available slot within bag
						return (base_slot_id + j);
				}
			}
		}
	}
	
	if (try_cursor)
		// Always room on cursor (it's a queue)
		// (we may wish to cap this in the future)
		return SLOT_CURSOR;
	
	// No available slots
	return SLOT_INVALID;
}

void Inventory::dumpInventory() {
	iter_inst it;
	iter_bag itb;
	ItemInst* inst = NULL;
	
	// Check item: After failed checks, check bag contents (if bag)
	printf("Worn items:\n");
	for (it=m_worn.begin(); it!=m_worn.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->Name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
						baginst->GetItem()->Name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
			}
		}
	}
	
	printf("Inventory items:\n");
	for (it=m_inv.begin(); it!=m_inv.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->Name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
							baginst->GetItem()->Name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
				
			}
		}
	}
	
	printf("Bank items:\n");
	for (it=m_bank.begin(); it!=m_bank.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->Name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
						baginst->GetItem()->Name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
				
			}
		}
	}
	
	printf("Shared Bank items:\n");
	for (it=m_shbank.begin(); it!=m_shbank.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->Name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
						baginst->GetItem()->Name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
				
			}
		}
	}
	
	printf("\n");
	fflush(stdout);
}

// Internal Method: Retrieves item within an inventory bucket
ItemInst* Inventory::_GetItem(const map<sint16, ItemInst*>& bucket, sint16 slot_id) const
{
	iter_inst it = bucket.find(slot_id);
	if (it != bucket.end()) {
		return it->second;
	}
	
	// Not found!
	return NULL;
}

// Internal Method: "put" item into bucket, without regard for what is currently in bucket
// Assumes item has already been allocated
sint16 Inventory::_PutItem(sint16 slot_id, ItemInst* inst)
{
	// If putting a NULL into slot, we need to remove slot without memory delete
	if (inst == NULL) {
		PopItem(slot_id);
		return slot_id;
	}
	
	sint16 result = SLOT_INVALID;
	
	if (slot_id==SLOT_CURSOR) { // Cursor
		// Replace current item on cursor, if exists
		m_cursor.pop(); // no memory delete, clients of this function know what they are doing
		m_cursor.push_front(inst);
		result = slot_id;
	}
	else if ((slot_id>=0 && slot_id<=21) || (slot_id >= 400 && slot_id<=404)) { // Worn slots
		m_worn[slot_id] = inst;
		result = slot_id;
	}
	else if ((slot_id>=22 && slot_id<=29)) {
		m_inv[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id>=2000 && slot_id<=2015) { // Bank slots
		m_bank[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id>=2500 && slot_id<=2501) { // Shared bank slots
		m_shbank[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id>=3000 && slot_id<=3007) { // Trade window slots
		m_trade[slot_id] = inst;
		result = slot_id;
	}
	else {
		// Slot must be within a bag
		ItemInst* baginst = GetItem(Inventory::CalcSlotId(slot_id)); // Get parent bag
		if (baginst && baginst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)baginst;
			bag->_PutItem(Inventory::CalcBagIdx(slot_id), inst);
			result = slot_id;
		}
	}
	
	if (result == SLOT_INVALID) {
		LogFile->write(EQEMuLog::Error, "Inventory::_PutItem: Invalid slot_id specified (%i)", slot_id);
		safe_delete(inst); // Slot not found, clean up
	}
	
	return result;
}

// Internal Method: Checks an inventory bucket for a particular item
sint16 Inventory::_HasItem(map<sint16, ItemInst*>& bucket, const Item_Struct* item, uint8 quantity)
{
	iter_inst it;
	iter_bag itb;
	ItemInst* inst = NULL;
	uint8 quantity_found = 0;
	
	// Check item: After failed checks, check bag contents (if bag)
	for (it=bucket.begin(); it!=bucket.end(); it++) {
		inst = it->second;
		if (inst && (inst->GetItem() == item)) {
			quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return it->first;
		}
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst->GetItem() == item) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(it->first, itb->first);
				}
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
sint16 Inventory::_HasItem(ItemInstQueue& queue, const Item_Struct* item, uint8 quantity)
{
	iter_queue it;
	iter_bag itb;
	uint8 quantity_found = 0;
	
	// Read-only iteration of queue
	for (it=queue.begin(); it!=queue.end(); it++) {
		ItemInst* inst = *it;
		if (inst && (inst->GetItem() == item)) {
			quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return SLOT_CURSOR;
		}
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst->GetItem() == item) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(SLOT_CURSOR, itb->first);
				}
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory bucket for a particular item
sint16 Inventory::_HasItemByUse(map<sint16, ItemInst*>& bucket, uint8 use, uint8 quantity)
{
	iter_inst it;
	iter_bag itb;
	ItemInst* inst = NULL;
	uint8 quantity_found = 0;
	
	// Check item: After failed checks, check bag contents (if bag)
	for (it=bucket.begin(); it!=bucket.end(); it++) {
		inst = it->second;
		if (inst && inst->IsType(ItemTypeCommon) && inst->GetItem()->Common.ItemUse == use) {
			quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return it->first;
		}
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst && baginst->IsType(ItemTypeCommon) && baginst->GetItem()->Common.ItemUse == use) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(it->first, itb->first);
				}
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
sint16 Inventory::_HasItemByUse(ItemInstQueue& queue, uint8 use, uint8 quantity)
{
	iter_queue it;
	iter_bag itb;
	uint8 quantity_found = 0;
	
	// Read-only iteration of queue
	for (it=queue.begin(); it!=queue.end(); it++) {
		ItemInst* inst = *it;
		if (inst && inst->IsType(ItemTypeCommon) && inst->GetItem()->Common.ItemUse == use) {
			quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return SLOT_CURSOR;
		}
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemTypeContainer)) {
			ItemContainerInst* bag = (ItemContainerInst*)inst;
			
			for (itb=bag->_begin(); itb!=bag->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst && baginst->IsType(ItemTypeCommon) && baginst->GetItem()->Common.ItemUse == use) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(SLOT_CURSOR, itb->first);
				}
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Return base item data without delim at end
string ItemInst::Serialize(sint16 slot_id) const
{
	_CP(ItemInst_Serialize);
	if (!m_item)
		return "";
	
	char ch[250] = {0}; // Estimate on largest possible
	
	// Format pipe-delimited string for packet
	int charges=m_charges;
	if(charges==255)
		charges=-1;
	int32 spellcharges = m_item->SpellCharges;
	if(spellcharges && m_item->Common.SpellId > 0 && m_item->Common.SpellId<65000 && m_item->Charges <= 1)
		spellcharges = charges;
	if(charges==-1)
		spellcharges = charges;
	sprintf(ch,
		"%i|%i|%i|%i|%i|%i|%i|%i|%i|\"%i|%s|%s|%s|%i|%i|%i|%i|%i|%i|%i|%i",
		charges,
		m_item->Unknown001,
		slot_id,
		m_price,
		m_item->Unknown004,
		(m_merchantslot==0) ? m_item->Unknown005 : m_merchantslot,
		m_item->Unknown006,
		spellcharges,
		m_item->Attuneable,
		m_item->ItemClass,
		m_item->Name,
		m_item->LoreName,
		m_item->IDFile,
		m_item->ItemNumber,
		m_item->Weight,
		m_item->NoRent,
		m_item->NoDrop,
		m_item->Size,
		m_item->EquipSlots,
		m_item->Cost,
		m_item->IconNumber);
	
	return ch;
}

// Serialize to a packet string
string ItemCommonInst::Serialize(sint16 slot_id) const
{
	_CP(ItemCommonInst_Serialize);
	if (!m_item)
		return "";
	
	char ch[1000] = {0}; // Estimate on largest packet
	const ItemCommon_Struct* common = &m_item->Common;
	const Item_Struct* item = m_item;
	string serialized,subitem;
	
	sprintf(ch,
		"%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|"	// ended with Deity
		"%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|"	// ended with Color
		"%i|%i|%i|%i|%i|%i|%i|%6.6f|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|"	// ended with SpellShield
		"%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|"	// ended with Unknown100
		"%i|%i|%i|%i|%i|%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|"       //
		"%i|%i|%i|%i|%i|%i|%i|%i|%i\"",						// bag/books stuff
		ItemInst::Serialize(slot_id).c_str(),
		common->Unknown021,
		common->Unknown022,
		common->Unknown023,
		common->Tradeskills,
		common->SvCold,
		common->SvDisease,
		common->SvPoison,
		common->SvMagic,
		common->SvFire,
		common->STR,
		common->STA,
		common->AGI,
		common->DEX,
		common->CHA,
		common->INT,
		common->WIS,
		common->HP,
		common->Mana,
		common->AC,
		common->Deity,
		// End of first row
		common->SkillModValue,
		common->SkillModType,
		common->BaneDmgRace,
		common->BaneDmg,
		common->BaneDmgBody,
		common->Magic,
		common->casttime2,
		common->ProcLevel,
		common->RequiredLevel,
		common->BardSkillType,
		common->BardSkillAmt,
		common->Light,
		common->Delay,
		common->RecommendedLevel,
		common->RecommendedSkill,
		common->ElemDmgType,
		common->ElemDmg,
		common->EffectType,
		common->Range,
		common->Damage,
		m_color,
		// End of second row
		common->Classes,
		common->Races,
		common->Unknown064,
		common->SpellId,
		common->MaxCharges,
		common->ItemUse,
		common->Material,
		common->SellRate,
		common->Unknown070,
		common->CastTime,
		common->Unknown072,
		common->ProcRateMod,
		common->FocusId,
		common->CombatEffects,
		common->Shielding,
		common->StunResist,
		common->StrikeThrough,
		common->CombatSkill,
		common->CombatSkillDmg,
		common->SpellShield,
		// End of third row
		common->Avoidance,
		common->Accuracy,
		common->CharmFormula,
		common->FactionMod1,
		common->FactionMod2,
		common->FactionMod3,
		common->FactionMod4,
		common->FactionAmt1,
		common->FactionAmt2,
		common->FactionAmt3,
		common->FactionAmt4,
		common->CharmFile,
		common->augtype,
		common->AugSlotType[0],
		common->AugSlotType[1],
		common->AugSlotType[2],
		common->AugSlotType[3],
		common->AugSlotType[4],
		common->ldonpointtheme,
		common->ldonpointcost,
		common->ldonsold,
		//End of fourth row
		0,	// bagtype
		0,	// bagslots
		0,	// bagsize
		0,	// bagwr
		0,	// booktype
		0,	// unknown108
		"",	// filename
		item->banedmgamt2,
		item->augmentrestriction,
		item->loreflag,
		item->pendingloreflag,
		item->artifactflag,
		item->summonedflag,
		item->tribute,
		item->gm,
		item->endur,
		item->dotshielding,
		item->attackbonus,
		item->hpregen,
		item->manaregen,
		item->hastepercent,
		// End of fifth row
		item->damageshield,
		item->unknown125,
		item->unknown126,
		item->unknown127,
		item->distiller,
		item->unknown129,
		item->unknown130,
		item->unknown131,
		item->unknown132
		);
	serialized=ch;
	
	// Doodman:  Do ten even tho we will only have 5 augments.  We need to fill 10 fields
	//   6-10 should always be empty (well, 5-9, actually)
	for(uint8 i=0; i<10; i++) {
		serialized+="|";
		
		iter_augment it = m_augments.find(i);
		if (it != m_augments.end()) {
			const ItemCommonInst* common = it->second;
			if (common) {
				subitem=common->Serialize(slot_id);
				Protect(subitem,'"');
				serialized+='"';
				serialized+=subitem;
				serialized+='"';
			}
		}
	}
	
	return serialized;
}

// Serialize to a packet string
string ItemContainerInst::Serialize(sint16 slot_id) const
{
	_CP(ItemContainerInst_Serialize);
	if (!m_item)
		return "";
	
	char ch[4000] = {0}; // Estimate on largest packet
	const ItemContainer_Struct* container = &m_item->Container;
	string serialized,subitem;
	
	sprintf(ch,
		"%s|-1|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|"		// ended with Deity
		"0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|"		// ended with Color
		"0|0|0|-1|0|0|0|1.000000|0|0|0|0|0|0|0|0|0|0|0|0|"	// ended with SpellShield
		"0|0|0|0|0|0|0|0|0|0|0||0|0|0|0|0|0|0|0|"			// ended with Unknown100
		"0|%i|%i|%i|%i|0||0|0|0|0|0|\"",					// bag/books stuff
		ItemInst::Serialize(slot_id).c_str(),
		container->PackType,
		container->Slots,
		container->SizeCapacity,
		container->WeightReduction);
	
	serialized=ch;
	
	// Doodman:  Do ten even tho we will only have "TotalSlots" items.  We need to fill 10 fields
	//   Slots over TotalSlots should be empty
	for(uint8 i=0; i<10; i++) {
		serialized+="|";
		
		iter_bag it = m_contents.find(i);
		if (it != m_contents.end()) {
			const ItemInst* inst = it->second;
			if (inst) {
				subitem=inst->Serialize(0);
				Protect(subitem,'"');
				serialized+='"';
				serialized+=subitem;
				serialized+='"';
			}
		}
	}
	
	return serialized;
}

// Serialize to a packet string
string ItemBookInst::Serialize(sint16 slot_id) const
{
	if (!m_item)
		return "";
	
	char ch[1000] = {0}; // Estimate on largest packet
	const ItemBook_Struct* book = &m_item->Book;
	
	sprintf(ch,
		"%s|-1|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|"		// ended with Deity
		"0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|"		// ended with Color
		"0|0|0|-1|0|0|0|1.000000|0|0|0|0|0|0|0|0|0|0|0|0|"	// ended with SpellShield
		"0|0|0|0|0|0|0|0|0|0|0||0|0|0|0|0|0|0|0|0|"			// ended with Unknown100
		"0|0|0|0|%i|%i|%s|0|0|0|0|0|0|\"||||||||||",			// bag/books stuff
		ItemInst::Serialize(slot_id).c_str(),
		book->BookType,
		book->Unknown108,
		book->File);
	
	return ch;
}

// Calculate slot_id for an item within a bag
sint16 Inventory::CalcSlotId(sint16 bagslot_id, uint8 bagidx)
{
	if (!Inventory::SupportsContainers(bagslot_id)) {
		return SLOT_INVALID;
	}
	
	sint16 slot_id = SLOT_INVALID;
	
	if (bagslot_id==SLOT_CURSOR || bagslot_id==8000) // Cursor
		slot_id = IDX_CURSOR_BAG + bagidx;
	else if (bagslot_id>=22 && bagslot_id<=29) // Inventory slots
		slot_id = IDX_INV_BAG + (bagslot_id-22)*10 + bagidx;
	else if (bagslot_id>=2000 && bagslot_id<=2015) // Bank slots
		slot_id = IDX_BANK_BAG + (bagslot_id-2000)*10 + bagidx;
	else if (bagslot_id>=2500 && bagslot_id<=2501) // Shared bank slots
		slot_id = IDX_SHBANK_BAG + (bagslot_id-2500)*10 + bagidx;
	else if (bagslot_id>=3000 && bagslot_id<=3007) // Trade window slots
		slot_id = IDX_TRADE_BAG + (bagslot_id-3000)*10 + bagidx;
	
	return slot_id;
}

// Opposite of above: Get parent bag slot_id from a slot inside of bag
sint16 Inventory::CalcSlotId(sint16 slot_id)
{
	sint16 parent_slot_id = SLOT_INVALID;
	
	if (slot_id>=251 && slot_id<=330)
		parent_slot_id = IDX_INV + (slot_id-251) / 10;
	else if (slot_id>=331 && slot_id<=340)
		parent_slot_id = SLOT_CURSOR;
	else if (slot_id>=2000 && slot_id<=2015)
		parent_slot_id = IDX_BANK + (slot_id-2000) / 10;
	else if (slot_id>=2031 && slot_id<=2190)
		parent_slot_id = IDX_BANK + (slot_id-2031) / 10;
	else if (slot_id>=2531 && slot_id<=2550)
		parent_slot_id = IDX_SHBANK + (slot_id-2531) / 10;
	else if (slot_id>=3100 && slot_id<=3179)
		parent_slot_id = IDX_TRADE + (slot_id-3100) / 10;
	
	return parent_slot_id;
}

uint8 Inventory::CalcBagIdx(sint16 slot_id)
{
	uint8 index = 0;
	
	if (slot_id>=251 && slot_id<=330)
		index = (slot_id-251) % 10;
	else if (slot_id>=331 && slot_id<=340)
		index = (slot_id-331) % 10;
	else if (slot_id>=2000 && slot_id<=2015)
		index = (slot_id-2000) % 10;
	else if (slot_id>=2031 && slot_id<=2190)
		index = (slot_id-2031) % 10;
	else if (slot_id>=2531 && slot_id<=2550)
		index = (slot_id-2531) % 10;
	else if (slot_id>=3100 && slot_id<=3179)
		index = (slot_id-3100) % 10;
	else if (slot_id>=4000 && slot_id<=4009)
		index = (slot_id-4000) % 10;
	
	return index;
}

sint16 Inventory::CalcSlotFromMaterial(int8 material)
{
	switch(material)
	{
		case MATERIAL_HEAD:
			return SLOT_HEAD;
		case MATERIAL_CHEST:
			return SLOT_CHEST;
		case MATERIAL_ARMS:
			return SLOT_ARMS;
		case MATERIAL_BRACER:
			return SLOT_BRACER01;	// there's 2 bracers, only one bracer material
		case MATERIAL_HANDS:
			return SLOT_HANDS;
		case MATERIAL_LEGS:
			return SLOT_LEGS;
		case MATERIAL_FEET:
			return SLOT_FEET;
		case MATERIAL_PRIMARY:
			return SLOT_PRIMARY;
		case MATERIAL_SECONDARY:
			return SLOT_SECONDARY;
		default:
			return -1;
	}
}

int8 Inventory::CalcMaterialFromSlot(sint16 equipslot)
{
	switch(equipslot)
	{
		case SLOT_HEAD:
			return MATERIAL_HEAD;
		case SLOT_CHEST:
			return MATERIAL_CHEST;
		case SLOT_ARMS:
			return MATERIAL_ARMS;
		case SLOT_BRACER01:
		case SLOT_BRACER02:
			return MATERIAL_BRACER;
		case SLOT_HANDS:
			return MATERIAL_HANDS;
		case SLOT_LEGS:
			return MATERIAL_LEGS;
		case SLOT_FEET:
			return MATERIAL_FEET;
		case SLOT_PRIMARY:
			return MATERIAL_PRIMARY;
		case SLOT_SECONDARY:
			return MATERIAL_SECONDARY;
		default:
			return 0xFF;
	}
}


// Test whether a given slot can support a container item
bool Inventory::SupportsContainers(sint16 slot_id)
{
	if ((slot_id>=22 && slot_id<=30) ||		// Personal inventory slots
		(slot_id>=2000 && slot_id<=2015) ||	// Bank slots
		(slot_id>=2500 && slot_id<=2501) ||	// Shared bank slots
		(slot_id==SLOT_CURSOR) ||			// Cursor
		(slot_id>=3000 && slot_id<=3007))	// Trade window
		return true;
	return false;
}
