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

// @merth notes:
// These classes could be optimized with database reads/writes by storing
// a status flag indicating how object needs to interact with database

#ifndef __ITEM_H
#define __ITEM_H

class ItemInst;				// Item belonging to a client (contains info on item, dye, augments, charges, etc)
class ItemCommonInst;		// Instance of a common item
class ItemContainerInst;	// Instance of a container item
class ItemBookInst;			// Instance of a book item
class ItemInstQueue;		// Queue of ItemInst objects (i.e., cursor)
class Inventory;			// Character inventory
class ItemParse;			// Parses item packets

#include <string>
#include <vector>
#include <map>
#include <list>
using namespace std;
#include "../common/eq_packet_structs.h"

// Helper typedefs
typedef list<ItemInst*>::const_iterator					iter_queue;
typedef map<sint16, ItemInst*>::const_iterator			iter_inst;
typedef map<uint8, ItemCommonInst*>::const_iterator		iter_augment;
typedef map<uint8, ItemInst*>::const_iterator			iter_bag;

namespace ItemField {
	enum {
		serialization=0,
#define F(x) x,
#include "item_fieldlist.h"
#undef F
		updated
	};
};

// Indexing positions into item material arrays
#define MATERIAL_HEAD		0
#define MATERIAL_CHEST		1
#define MATERIAL_ARMS		2
#define MATERIAL_BRACER		3
#define MATERIAL_HANDS		4
#define MATERIAL_LEGS		5
#define MATERIAL_FEET		6
#define MATERIAL_PRIMARY	7
#define MATERIAL_SECONDARY	8
#define MAX_MATERIALS 9	//number of equipables

// Indexing positions to the beginning slot_id's for a bucket of slots
#define IDX_EQUIP		0
#define IDX_CURSOR_BAG	331
#define IDX_INV			22
#define IDX_INV_BAG		251
#define IDX_TRIBUTE		400
#define IDX_BANK		2000
#define IDX_BANK_BAG	2031
#define IDX_SHBANK		2500
#define IDX_SHBANK_BAG	2531
#define IDX_TRADE		3000
#define IDX_TRADE_BAG	3031
#define IDX_TRADESKILL	4000
#define MAX_ITEMS_PER_BAG 10

// Specifies usage type for item inside ItemInst
enum ItemUseType
{
	ItemUseNormal,
	ItemUseWorldContainer
};

//FatherNitwit: location bits for searching specific
//places with HasItem() and HasItemByUse()
enum {
	invWhereWorn 		= 0x01,
	invWherePersonal	= 0x02,	//in the character's inventory
	invWhereBank		= 0x04,
	invWhereSharedBank	= 0x08,
	invWhereTrading		= 0x10,
	invWhereCursor		= 0x20
};

/*
** Inventory Slot Equipment Enum
** Mostly used for third-party tools to reference inventory slots
**
** NOTE: Numbering for personal inventory goes top to bottom, then left to right
**	It's the opposite for inside bags: left to right, then top to bottom
**	Example:
**	inventory:	containers:
**	1 6			1 2
**	2 7			3 4
**	3 8			5 6
**	4 9			7 8
**	5 10		9 10
**
*/
enum InventorySlot
{
	////////////////////////
	// Equip slots
	////////////////////////
	
	SLOT_CHARM		= 0,
	SLOT_EAR01		= 1,
	SLOT_HEAD		= 2,
	SLOT_FACE		= 3,
	SLOT_EAR02		= 4,
	SLOT_NECK		= 5,
	SLOT_SHOULDER	= 6,
	SLOT_ARMS		= 7,
	SLOT_BACK		= 8,
	SLOT_BRACER01	= 9,
	SLOT_BRACER02	= 10,
	SLOT_RANGE		= 11,
	SLOT_HANDS		= 12,
	SLOT_PRIMARY	= 13,
	SLOT_SECONDARY	= 14,
	SLOT_RING01		= 15,
	SLOT_RING02		= 16,
	SLOT_CHEST		= 17,
	SLOT_LEGS		= 18,
	SLOT_FEET		= 19,
	SLOT_WAIST		= 20,
	SLOT_AMMO		= 21,
	
	////////////////////////
	// All other slots
	////////////////////////
	SLOT_PERSONAL_BEGIN = 22,
	SLOT_PERSONAL_END = 29,
	
	SLOT_CURSOR		= 30,
	
	SLOT_CURSOR_END	= (sint16)0xFFFE,	// Last item on cursor queue
	// Cursor bag slots are 331->340 (10 slots)
	
	// Personal Inventory Slots
	// Slots 1 through 8 are slots 22->29
	// Inventory bag slots are 251->330 (10 slots per bag)
	
	// Tribute slots are 400-404? (upper bound unknown)
	// storing these in worn item's map
	
	// Bank slots
	// Bank slots 1 through 16 are slots 2000->2015
	// Bank bag slots are 2031->2190
	
	// Shared bank slots
	// Shared bank slots 1 through 2 are slots 2500->2501
	// Shared bank bag slots are 2531->2550
	
	// Trade session slots
	// Trade slots 1 through 8 are slots 3000->3007
	// Trade bag slots are technically 0->79 when passed to client,
	// but in our code, we treat them as slots 3100->3179
	
	// Slot used in OP_TradeSkillCombine for world tradeskill containers
	SLOT_TRADESKILL	= 1000,
	
	// Value recognized by client for destroying an item
	SLOT_INVALID = (sint16)0xFFFF
};


// ########################################
// Class: Queue
//	Queue that allows a read-only iterator
class ItemInstQueue
{
public:
	~ItemInstQueue();
	/////////////////////////
	// Public Methods
	/////////////////////////
	
	inline iter_queue begin()	{ return m_list.begin(); }
	inline iter_queue end()		{ return m_list.end(); }
	
	void push(ItemInst* inst);
	void push_front(ItemInst* inst);
	ItemInst* pop();
	ItemInst* peek_front() const;
	inline int size() 		{ return m_list.size(); }
	
protected:
	/////////////////////////
	// Protected Members
	/////////////////////////
	
	list<ItemInst*> m_list;
	
};

// ########################################
// Class: Inventory
//	Character inventory
class Inventory
{
	friend class ItemInst;
public:
	///////////////////////////////
	// Public Methods
	///////////////////////////////
	
	virtual ~Inventory();
	
	// Retrieve a writeable item at specified slot
	ItemInst* GetItem(sint16 slot_id) const;
	ItemInst* GetItem(sint16 slot_id, uint8 bagidx) const;

	inline iter_queue cursor_begin()	{ return m_cursor.begin(); }
	inline iter_queue cursor_end()		{ return m_cursor.end(); }
	
	// Retrieve a read-only item from inventory
	inline const ItemInst* operator[](sint16 slot_id) const { return GetItem(slot_id); }
	
	// Add item to inventory
	sint16 PutItem(sint16 slot_id, const ItemInst& inst);

	// Add item to cursor queue
	sint16 PushCursor(const ItemInst& inst);
	
	// Swap items in inventory
	void SwapItem(sint16 slot_a, sint16 slot_b);

	// Remove item from inventory
	void DeleteItem(sint16 slot_id, uint8 quantity=0);
	
	// Remove item from inventory (and take control of memory)
	ItemInst* PopItem(sint16 slot_id);
	
	// Check whether item exists in inventory
	// where argument specifies OR'd list of invWhere constants to look
	sint16 HasItem(uint32 item_id, uint8 quantity=0, uint8 where=0xFF);
	
	// Check whether item exists in inventory
	// where argument specifies OR'd list of invWhere constants to look
	sint16 HasItemByUse(uint8 use, uint8 quantity=0, uint8 where=0xFF);
	
	// Locate an available inventory slot
	sint16 FindFreeSlot(bool for_bag, bool try_cursor, int8 min_size = 0);
	
	// Calculate slot_id for an item within a bag
	static sint16 CalcSlotId(sint16 slot_id); // Calc parent bag's slot_id
	static sint16 CalcSlotId(sint16 bagslot_id, uint8 bagidx); // Calc slot_id for item inside bag
	static uint8 CalcBagIdx(sint16 slot_id); // Calc bagidx for slot_id
	static sint16 CalcSlotFromMaterial(int8 material);
	static int8 CalcMaterialFromSlot(sint16 equipslot);


	// Test whether a given slot can support a container item
	static bool SupportsContainers(sint16 slot_id);
	
	void dumpInventory();
	
protected:
	///////////////////////////////
	// Protected Methods
	///////////////////////////////
	
	// Retrieves item within an inventory bucket
	ItemInst* _GetItem(const map<sint16, ItemInst*>& bucket, sint16 slot_id) const;
	
	// Private "put" item into bucket, without regard for what is currently in bucket
	sint16 _PutItem(sint16 slot_id, ItemInst* inst);
	
	// Checks an inventory bucket for a particular item
	sint16 _HasItem(map<sint16, ItemInst*>& bucket, const Item_Struct* item, uint8 quantity);
	sint16 _HasItem(ItemInstQueue& iqueue, const Item_Struct* item, uint8 quantity);
	sint16 _HasItemByUse(map<sint16, ItemInst*>& bucket, uint8 use, uint8 quantity);
	sint16 _HasItemByUse(ItemInstQueue& iqueue, uint8 use, uint8 quantity);
	
	
	// Player inventory
	map<sint16, ItemInst*>	m_worn;		// Items worn by character
	map<sint16, ItemInst*>	m_inv;		// Items in character personal inventory
	map<sint16, ItemInst*>	m_bank;		// Items in character bank
	map<sint16, ItemInst*>	m_shbank;	// Items in character shared bank
	map<sint16, ItemInst*>	m_trade;	// Items in a trade session
	ItemInstQueue			m_cursor;	// Items on cursor: FIFO
};



// ########################################
// Class: ItemInst
//	Base class for an instance of an item
//	An item instance encapsulates item data + data specific
//	to an item instance (includes dye, augments, charges, etc)
class ItemInst
{
public:
	/////////////////////////
	// Methods
	/////////////////////////
	
	// Constructors/Destructor
	ItemInst(const Item_Struct* item = NULL, const unsigned char* item_s = NULL, sint16 charges = 0) {
		m_use_type = ItemUseNormal;
		m_item = item;
		m_item_serialization = item_s;
		m_charges = charges;
		m_price = 0;
		m_instnodrop = false;
		m_merchantslot = 0;
		if(m_item &&m_item->ItemClass == ItemClassCommon)
			m_color = m_item->Common.Color;
		else
			m_color = 0;
	}
	
	ItemInst(const Item_Struct* item = NULL, sint16 charges = 0);
	
	ItemInst(uint32 item_id, sint16 charges = 0);
	
	ItemInst(ItemUseType use_type) {
		m_use_type = use_type;
		m_item = NULL;
		m_item_serialization = NULL;
		m_charges = 0;
		m_price = 0;
		m_instnodrop = false;
		m_merchantslot = 0;
		m_color = 0;
	}
	
	virtual ~ItemInst() {}
	
	// Query item type
	virtual bool IsType(ItemClass item_class) const;
	
	// Can item be stacked?
	virtual bool IsStackable() const;

	// Can item be equipped by/at?
	virtual bool IsEquipable(int16 race, int16 class_) const;
	virtual bool IsEquipable(sint16 slot_id) const;
	
	// Has attack/delay?
	virtual bool IsWeapon() const;

	//  Virtual function, so anyone can call it
	virtual uint32 GetAugmentItemID(uint8 slot) const { return 0; }
	
	// Serialize into a pipe-delimited string for packet
	virtual string Serialize(sint16 slot_id) const;
	
	// Accessors
	const uint32 GetID() const { return m_item->ID; }
	const Item_Struct* GetItem() const		{ return m_item; }
	void SetItem(const Item_Struct* item)	{ m_item = item; }
	
	sint16 GetCharges() const				{ return m_charges; }
	void SetCharges(sint16 charges)			{ m_charges = charges; }
	
	uint32 GetPrice() const					{ return m_price; }
	void SetPrice(uint32 price)				{ m_price = price; }
	
	void SetColor(uint32 color)				{ m_color = color; }
	uint32 GetColor() const					{ return m_color; }

	uint32 GetMerchantSlot() const				{ return m_merchantslot; }
	void SetMerchantSlot(uint32 slot)		{ m_merchantslot = slot; }

	sint32 GetMerchantCount() const				{ return m_merchantcount; }
	void SetMerchantCount(sint32 count)		{ m_merchantcount = count; }

	sint16 GetCurrentSlot() const			{ return m_currentslot; }
	void SetCurrentSlot(sint16 curr_slot)   { m_currentslot = curr_slot; }

	// Is this item already attuned?
	bool IsInstNoDrop() const { return m_instnodrop; }
	void SetInstNoDrop(bool flag) { m_instnodrop=flag; }

	// Allows treatment of this object as though it were a pointer to m_item
	operator bool() const { return (m_item != NULL); }
	
	// Compare inner Item_Struct of two ItemInst objects
	bool operator==(const ItemInst& right) const { return (this->m_item == right.m_item); }
	bool operator!=(const ItemInst& right) const { return (this->m_item != right.m_item); }
	
	// Clone current item
	virtual ItemInst* Clone() const = 0;
	
	// Create appropriate ItemInst class
	static ItemInst* Create(uint32 item_id, sint16 charges=0, uint32 aug1=0, uint32 aug2=0, uint32 aug3=0, uint32 aug4=0, uint32 aug5=0);
	static ItemInst* Create(const Item_Struct* item, sint16 charges=0, uint32 aug1=0, uint32 aug2=0, uint32 aug3=0, uint32 aug4=0, uint32 aug5=0);
	
	bool IsSlotAllowed(sint16 slot_id) const;
protected:
	//////////////////////////
	// Protected Members
	//////////////////////////
	
	ItemUseType			m_use_type;	// Usage type for item
	const Item_Struct*	m_item;		// Ptr to item data
	const unsigned char*	m_item_serialization;		// Ptr to item serialization
	sint16				m_charges;	// # of charges for chargeable items
	uint32				m_price;	// Bazaar /trader price
	uint32				m_color;
	uint32				m_merchantslot;
	sint16				m_currentslot;
	bool 				m_instnodrop;
	sint32				m_merchantcount;		//number avaliable on the merchant, -1=unlimited
};




// ########################################
// Class: ItemCommonInst
//	Common item instanced data
class ItemCommonInst : public ItemInst
{
public:
	/////////////////////////
	// Methods
	/////////////////////////
	
	// Constructors/Destructor
	ItemCommonInst(const Item_Struct* item = NULL, sint16 charges = 0, uint32 aug1 = 0, uint32 aug2 = 0, uint32 aug3 = 0, uint32 aug4 = 0, uint32 aug5 = 0);
	ItemCommonInst(const Item_Struct* item = NULL, const unsigned char* item_s = NULL, sint16 charges = 0, uint32 aug1 = 0, uint32 aug2 = 0, uint32 aug3 = 0, uint32 aug4 = 0, uint32 aug5 = 0);
	ItemCommonInst(uint32 item_id, sint16 charges = 0, uint32 aug1 = 0, uint32 aug2 = 0, uint32 aug3 = 0, uint32 aug4 = 0, uint32 aug5 = 0);
	ItemCommonInst(const ItemCommonInst& copy);
	virtual ~ItemCommonInst();
	
	// Can item be stacked?
	virtual bool IsStackable() const;
	
	// Can item be equipped by/at?
	virtual bool IsEquipable(int16 race, int16 class_) const;
	virtual bool IsEquipable(sint16 slot_id) const;

	// Augements
	inline bool IsAugmentable() const { return m_item->Common.AugSlotType[0]!=0; }
	sint8 AvailableAugmentSlot(sint32 augtype) const;
	inline sint32 GetAugmentType() const { return m_item->Common.AugType; }
	
	// Has attack/delay?
	virtual bool IsWeapon() const;
	
	// Retrieve a writeable augment from item
	ItemCommonInst* GetAugment(uint8 slot) const;

	// Retrieve an augments itemid;
	uint32 GetAugmentItemID(uint8 slot) const;
	
	// Retrieve a read-only augment from item
	inline const ItemCommonInst* operator[](uint8 slot) const { return GetAugment(slot); }
	
	// Add an augment to the item
	void PutAugment(uint8 slot, const ItemCommonInst& augment);
	void PutAugment(uint8 slot, uint32 item_id);
	
	// Remove augment from item and destroy it
	void DeleteAugment(uint8 slot);

	// Remove augment from item and return it
	ItemCommonInst* ItemCommonInst::RemoveAugment(uint8 index);
	
	// Serialize into a pipe-delimited string for packet
	virtual string Serialize(sint16 slot_id) const;
	
	// Clone current item
	virtual ItemInst* Clone() const;

	bool IsSlotAllowed(sint16 slot_id);
	
	
protected:
	//////////////////////////
	// Protected Members
	//////////////////////////
	
	iter_augment _begin()		{ return m_augments.begin(); }
	iter_augment _end()			{ return m_augments.end(); }
	
	// Put new augment, regardless of whether something exists there or not
	void _PutAugment(uint8 slot, ItemCommonInst* inst)	{ m_augments[slot] = inst; }
//GCC dosent like this cause its protected
//	friend sint16 Inventory::_PutItem(sint16, ItemInst*);
	friend class Inventory;
	
	map<uint8, ItemCommonInst*>	m_augments;	// LDoN augments on this item
	
};

typedef enum {
	byFlagIgnore,	//do not consider this flag
	byFlagSet,		//apply action if the flag is set
	byFlagNotSet	//apply action if the flag is NOT set
} byFlagSetting;

// ########################################
// Class: ItemContainerInst
//	Container item instanced data
class ItemContainerInst : public ItemInst
{
public:
	/////////////////////////
	// Public Methods
	/////////////////////////
	
	// Constructors/Destructor
	ItemContainerInst(const Item_Struct* item = NULL, const unsigned char* item_s = NULL, sint16 charges = 0) : ItemInst(item, item_s, charges) {}
	ItemContainerInst(const Item_Struct* item = NULL, sint16 charges = 0) : ItemInst(item, charges) {}
	ItemContainerInst(uint32 item_id, sint16 charges = 0) : ItemInst(item_id, charges) {}
	ItemContainerInst(ItemUseType use_type) : ItemInst(use_type) {}
	ItemContainerInst(const ItemContainerInst& copy);
	
	virtual ~ItemContainerInst();
	
	// Serialize into a pipe-delimited string for packet
	virtual string Serialize(sint16 slot_id) const;
	
	// Retrieve writeable item from container
	ItemInst* GetItem(uint8 index) const;
	
	// Retrieve a read-only item from bag
	inline const ItemInst* operator[](uint8 index) const { return GetItem(index); }
	
	// Add an item to container
	void PutItem(uint8 index, const ItemInst& inst);
	
	// "Pop" an item out of a container and take ownership of the memory
	ItemInst* PopItem(uint8 index);
	
	// Delete item in container
	void DeleteItem(uint8 index);
	
	// Remove all items from container
	void Clear();
	
	//Remove items based on their flags
	//the three flag types (nodrop, norent, flags) are ORed
	//but all the flags in flags_set must be set to match that
	void ClearByFlags(byFlagSetting is_nodrop, byFlagSetting is_norent);
	
	// Query item type
	virtual bool IsType(ItemClass item_class) const;
	
	// Clone current item
	virtual ItemInst* Clone() const;

	uint8 FirstOpenSlot() const;
	
	
protected:
	//////////////////////////
	// Protected Methods
	//////////////////////////
	
	iter_bag _begin()		{ return m_contents.begin(); }
	iter_bag _end()			{ return m_contents.end(); }

/*	GCC 4 dosent like this cause they are protected
	friend sint16 Inventory::_HasItem(map<sint16, ItemInst*>& bucket, const Item_Struct* item, uint8 quantity);
	friend sint16 Inventory::_HasItem(ItemInstQueue& queue, const Item_Struct* item, uint8 quantity);
	friend sint16 Inventory::_HasItemByUse(map<sint16, ItemInst*>& bucket, uint8 use, uint8 quantity);
	friend sint16 Inventory::_HasItemByUse(ItemInstQueue& queue, uint8 use, uint8 quantity);
*/
	friend class Inventory;

	friend void Inventory::dumpInventory();
	
	// Add pre-allocated item to container .. container now owns this memory
	void _PutItem(uint8 index, ItemInst* inst) { m_contents[index] = inst; }
// GCC4	friend sint16 Inventory::_PutItem(sint16, ItemInst*);
	
	// Items inside of this container
	map<uint8, ItemInst*> m_contents; // Zero-based index: min=0, max=9
	
};



// ########################################
// Class: ItemBookInst
//	Book item instanced data
class ItemBookInst : public ItemInst
{
public:
	/////////////////////////
	// Public Methods
	/////////////////////////
	
	// Constructors/Destructor
	ItemBookInst(const Item_Struct* item = NULL, const unsigned char* item_s = NULL, sint16 charges = 0) : ItemInst(item, item_s, charges) {}
	ItemBookInst(const Item_Struct* item = NULL, sint16 charges = 0) : ItemInst(item, charges) {}
	ItemBookInst(uint32 item_id, sint16 charges = 0) : ItemInst(item_id, charges) {}
	virtual ~ItemBookInst() {}
	
	// Serialize into a pipe-delimited string
	virtual string Serialize(sint16 slot_id) const;
	
	// Clone current item
	virtual ItemInst* Clone() const;
	
	
protected:
	/////////////////////////
	// Protected Methods
	/////////////////////////
	
};



#endif // #define __ITEM_H
