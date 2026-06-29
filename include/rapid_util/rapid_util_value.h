// Copyright (C) 2025 Liu Wu. All rights reserved.
//
// Licensed under the zlib License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/Zlib
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.


#ifndef __RAPID_UTIL_VALUE_H__
#define __RAPID_UTIL_VALUE_H__

#include <variant>
#include <functional>
#include "rapid_util_preprocessor.h"

namespace rapidjson_util {

namespace detail {


template<typename WantedType, typename GivenType>
struct GetIfMatched {
	static WantedType get(GivenType a) {
		if constexpr (std::is_same_v<std::remove_cv_t<WantedType>,
			std::remove_cv_t<GivenType>>) {
			return a;
		}
		else {
			/* Should never be reached - this is the fallback for non-matching types */
			assert(false);
			return WantedType{};
		}
	}
};


template<typename Target, typename Source>
struct AssignIfMatched {
	static void assign(Target& to, const Source& from)
	{
		/* Should never be reached - this is the fallback for non-matching types */
		assert(false);
	}
};

template<typename Type>
struct AssignIfMatched<Type, Type> {
	static void assign(Type& to, const Type& from) {
		to = from;
	}
};

template<>
struct AssignIfMatched<std::string, std::string_view> {
	static void assign(std::string& to, std::string_view from) {
		to = from;
	}
};


template<typename Type>
struct AccessPolicy {
	using ValueType = Type;
	using MemberPointerType = Type*;

	explicit AccessPolicy(MemberPointerType p) : ptr(p) {}

	bool canBeNone() const { return false; }
	bool isNone() const { return false; }
	void setNone() {}
	void reinit() { if constexpr (!std::is_const_v<ValueType>) { *ptr = ValueType{}; } }

	ValueType& value() const { return *ptr; }
	MemberPointerType pointer() const { return ptr; }

private:
	MemberPointerType ptr;
};

template<typename Type>
struct AccessPolicy<std::shared_ptr<Type>> {
	using ValueType = Type;
	using MemberPointerType = std::shared_ptr<Type>*;

	explicit AccessPolicy(MemberPointerType p) : ptr(p) {}

	bool canBeNone() const { return true; }
	bool isNone() const { return (*ptr) == nullptr; }
	void setNone() { (*ptr).reset(); }
	void reinit() { if constexpr (!std::is_const_v<ValueType>) { (*ptr) = std::make_shared<ValueType>(); } }

	ValueType& value()          const { return *(ptr->get()); }
	MemberPointerType pointer() const { return ptr; }

private:
	MemberPointerType ptr;
};

template<typename Type>
struct AccessPolicy<const std::shared_ptr<Type>> {
	using ValueType = Type;
	using MemberPointerType = const std::shared_ptr<Type>*;

	explicit AccessPolicy(MemberPointerType p) : ptr(p) {}

	bool canBeNone() const { return false; }
	bool isNone() const { return (*ptr) == nullptr; }
	void setNone() { }
	void reinit() { }

	ValueType& value()          const { return *(ptr->get()); }
	MemberPointerType pointer() const { return ptr; }

private:
	MemberPointerType ptr;
};

template<typename Type>
struct AccessPolicy<std::optional<Type>> {
	using ValueType = Type;
	using MemberPointerType = std::optional<Type>*;

	explicit AccessPolicy(MemberPointerType p) : ptr(p) {}

	bool canBeNone() const { return true; }
	bool isNone()    const { return (*ptr) == std::nullopt; }
	void setNone() { (*ptr).reset(); }
	void reinit() { if constexpr (!std::is_const_v<ValueType>) { (*ptr) = ValueType{}; } }

	ValueType& value()          const { return (*ptr).value(); }
	MemberPointerType pointer() const { return ptr; }

private:
	MemberPointerType ptr;
};

template<typename Type>
struct AccessPolicy<const std::optional<Type>> {
	using ValueType = const Type;
	using MemberPointerType = const std::optional<Type>*;

	explicit AccessPolicy(MemberPointerType p) : ptr(p) {}

	bool canBeNone() const { return false; }
	bool isNone() const { return (*ptr) == std::nullopt; }
	void setNone() { }
	void reinit() { }

	ValueType& value() const { return (*ptr).value(); }
	MemberPointerType pointer() const { return ptr; }

private:
	MemberPointerType ptr;
};


struct PrimitiveTag {};
struct StructTag {};
struct SequentialContainerTag {};
struct TupleTag {};

template<typename T, typename U = std::void_t<>>
struct Type2Tag;

template<typename T>
struct Type2Tag<T, std::enable_if_t<is_jsonable_primitive_type_v<T>>> {
	using Tag = PrimitiveTag;
};

template<typename T>
struct Type2Tag<T, std::enable_if_t<is_jsonable_struct_v<T>>> {
	using Tag = StructTag;
};

template<typename T>
struct Type2Tag<T, std::enable_if_t<is_jsonable_sequential_container_v<T>>> {
	using Tag = SequentialContainerTag;
};

template<typename T>
struct Type2Tag<T, std::enable_if_t<is_jsonable_tuple_v<T>>> {
	using Tag = TupleTag;
};


template<bool, typename> class Primitive;
template<bool, typename> class Object;
template<bool, typename> class Array;

/**
 * @brief This class is a variant that represents the internal tree structure of a struct type which
 *        is tagged by the RAPIDJSON_UTIL_DESCRIBE_MEMBERS macro. It provides a way to introspect and
 *        modify struct members at runtime by storing member pointers
 *
 */
class Value {
public:
	struct Member;
	using MemberIterator = std::vector<Member>::iterator;
	using ConstMemberIterator = std::vector<Member>::const_iterator;
	using ArrayIterator = std::vector<Value>::iterator;
	using ConstArrayIterator = std::vector<Value>::const_iterator;
	using PrimitiveType = Primitive<false, Value>;
	using ConstPrimitiveType = Primitive<true, Value>;
	using ObjectType = Object<false, Value>;
	using ConstObjectType = Object<true, Value>;
	using ArrayType = Array<false, Value>;
	using ConstArrayType = Array<true, Value>;
	
	/**
	*  @brief This constructor creates a Value object that internally stores a pointer to the
	*         user's JSON-serializable data. Any modifications made through the Value's interface
	*         (e.g., setInt(), resize(), member access) will be directly applied to the pointed object
	* 
	*  @param ptr Pointer to the source object
	* 
    *  @see     is_jsonable_v template
	*/
	template<typename Type>
	Value(Type* ptr);


	/**
	  * @brief Checks if the underlying value is modifiable, if Type in Value constructor is a const pointer,
	  *        then it return false otherwise true
	  */
	bool isModifiable() const { return valuePtr()->isModifiable(); }


	bool isPrimitive() const { return std::holds_alternative<PrimHolderPtr>(held); }
	bool isObject()    const { return std::holds_alternative<ObjectHolderPtr>(held); }
	bool isArray()     const { return std::holds_alternative<ArrayHolderPtr>(held); }

	PrimitiveType asPrimitive();
	ArrayType     asArray();
	ObjectType    asObject();

	ConstPrimitiveType asConstPrimitive() const;
	ConstObjectType    asConstObject()    const;
	ConstArrayType     asConstArray()     const;


  /* ============================================================================
   * Common operators for all variants including primitive, object, and array types
   * ============================================================================ */
	/**
	 * @brief   Checks if the value can be set to none (missing a value)
	 * @return  true if the value can be null (e.g., with std::optional or std::shared_ptr wrapper),
	 *          false otherwise
	 */
	bool canBeNone() const { return isModifiable() && valuePtr()->canBeNone(); }

	/**
	 * @brief   Checks if the value is currently none (missing a value)
	 * @note    If the struct member is not wrapped with std::optional or std::shared_ptr,
	 *          it always returns false (cannot be null)
	 */
	bool isNone() const { return valuePtr()->isNone(); }

	/**
	 * @brief Sets the underlying value to none (missing a value)
	 */
	void setNone() { assert(isModifiable() && canBeNone()); valuePtr()->setNone(); }

	/**
	 * @brief Reinitializes the value to its default state
	 */
	void reinit() { assert(isModifiable()); valuePtr()->reinit(); }


	/* ============================================================================
	 * Specific operators for object (struct/class) types
	 * ============================================================================ */

	 /// Returns iterator to the first member of the object
	MemberIterator memberBegin() { assert(isObject()); return objPtr()->memberBegin(); }

	/// Returns iterator to the end (past-the-last) of the object's members
	MemberIterator memberEnd() { assert(isObject()); return objPtr()->memberEnd(); }

	ConstMemberIterator memberBegin() const { assert(isObject()); return objPtr()->memberBegin(); }

	ConstMemberIterator memberEnd() const { assert(isObject()); return objPtr()->memberEnd(); }


	/* ============================================================================
	 * Specific operators for array (sequential or tuple-based) types
	 * ============================================================================ */

	 /**
       * @brief   Checks if the array can contain null elements
       * @return  true if elements can be null (e.g., std::vector<std::optional<...>>,
       *                                              std::list<std::shared_ptr<...>>)
       */
	bool canHoldNoneElem() const { return isArray() && arrayPtr()->canHoldNoneElem(); }

	 /**
	  * @brief Checks if the array can be resized (dynamic size)
	  * @return true if the array supports resize operations (e.g., std::vector, std::list),
	  *         false for fixed-size arrays (e.g., std::array, std::tuple)
	  */
	bool isResizable() const { return isArray() && arrayPtr()->isResizable(); }

	/**
	 * @brief Returns the current number of elements in the array
	 */
	std::size_t size() const { assert(isArray()); return arrayPtr()->size(); }

	/**
	 * @brief Resizes the array to contain newSize elements
	 */
	void resize(std::size_t newSize) {
		assert(isModifiable() && isArray() && isResizable());
		arrayPtr()->resize(newSize);
	}

	/// Returns iterator to the first element of the array
	ArrayIterator arrayBegin() { assert(isArray()); return arrayPtr()->arrayBegin(); }

	/// Returns iterator to the end (past-the-last) of the array
	ArrayIterator arrayEnd() { assert(isArray()); return arrayPtr()->arrayEnd(); }

	ConstArrayIterator arrayBegin() const { assert(isArray()); return arrayPtr()->arrayBegin(); }

	ConstArrayIterator arrayEnd() const { assert(isArray()); return arrayPtr()->arrayEnd(); }


	/* ============================================================================
	 * Specific operators for primitive types
	 * ============================================================================ */

	 /// @name Type inspection
	 /// @{
	bool isBool()   const { auto p = primPtr(); return p && p->isBool(); }
	bool isInt()    const { auto p = primPtr(); return p && p->isInt(); }
	bool isUint()   const { auto p = primPtr(); return p && p->isUint(); }
	bool isInt64()  const { auto p = primPtr(); return p && p->isInt64(); }
	bool isUint64() const { auto p = primPtr(); return p && p->isUint64(); }
	bool isFloat()  const { auto p = primPtr(); return p && p->isFloat(); }
	bool isDouble() const { auto p = primPtr(); return p && p->isDouble(); }
	bool isString() const { auto p = primPtr(); return p && p->isString(); }
	/// @}

	/// @name Getters (require matching type)
	/// @{
	bool        getBool()   const { auto p = primPtr(); assert(p && p->isBool());   return p->getBool(); }
	int         getInt()    const { auto p = primPtr(); assert(p && p->isInt());    return p->getInt(); }
	unsigned    getUint()   const { auto p = primPtr(); assert(p && p->isUint());   return p->getUint(); }
	int64_t     getInt64()  const { auto p = primPtr(); assert(p && p->isInt64());  return p->getInt64(); }
	uint64_t    getUint64() const { auto p = primPtr(); assert(p && p->isUint64()); return p->getUint64(); }
	float       getFloat()  const { auto p = primPtr(); assert(p && p->isFloat());  return p->getFloat(); }
	double      getDouble() const { auto p = primPtr(); assert(p && p->isDouble()); return p->getDouble(); }
	std::string getString() const { auto p = primPtr(); assert(p && p->isString()); return p->getString(); }
	/// @}

	/// @name Setters (require matching type and modifiability)
	/// @{
	void setBool(bool b)     { auto p = primPtr(); assert(p && isModifiable() && p->isBool());  p->setBool(b); }
	void setInt(int i)       { auto p = primPtr(); assert(p && isModifiable() && p->isInt());   p->setInt(i); }
	void setUint(unsigned u) { auto p = primPtr(); assert(p && isModifiable() && p->isUint());  p->setUint(u); }
	void setInt64(int64_t i) { auto p = primPtr(); assert(p && isModifiable() && p->isInt64()); p->setInt64(i); }
	void setUint64(uint64_t u) { auto p = primPtr(); assert(p && isModifiable() && p->isUint64()); p->setUint64(u); }
	void setFloat(float f)     { auto p = primPtr(); assert(p && isModifiable() && p->isFloat());  p->setFloat(f); }
	void setDouble(double d)   { auto p = primPtr(); assert(p && isModifiable() && p->isDouble()); p->setDouble(d); }
	void setString(std::string_view str) { auto p = primPtr(); assert(p && isModifiable() && p->isString()); p->setString(str); }
	/// @}


	Value(Value&& other) = default;
	Value& operator=(Value&& other) = default;
	~Value() = default;


private:
	class ValueHolder {
	public:
		virtual bool isModifiable() const = 0;
		virtual bool canBeNone()    const = 0;
		virtual bool isNone()       const = 0;
		virtual void setNone() = 0;
		virtual void reinit() = 0;

		virtual ~ValueHolder() = default;
	};

	template<typename Type, typename AccPolicy = AccessPolicy<Type>>
	class ValueHolderImpl : public virtual ValueHolder {
	public:
		using ValueType = typename AccPolicy::ValueType;
		using MemberPointerType = typename AccPolicy::MemberPointerType;

		ValueHolderImpl(Type* memberPtr) : accPolicy(memberPtr) {}

		bool isModifiable() const override { return !std::is_const_v<ValueType>; }
		bool canBeNone()    const override { return accPolicy.canBeNone(); }
		bool isNone()       const override { return accPolicy.isNone(); }
		void setNone()            override { return accPolicy.setNone(); }
		void reinit()             override { return accPolicy.reinit(); }

		virtual ~ValueHolderImpl() = default;

	protected:
		template<typename T>
		static constexpr bool isType() {
			return std::is_same_v<T, std::remove_const_t<ValueType>>;
		}

		ValueType& value()          const { return accPolicy.value(); }
		MemberPointerType pointer() const { return accPolicy.pointer(); }

		AccPolicy accPolicy;
	};

	class PrimitiveHolder : public virtual ValueHolder {
	public:
		virtual bool isBool()   const = 0;
		virtual bool isInt()    const = 0;
		virtual bool isUint()   const = 0;
		virtual bool isInt64()  const = 0;
		virtual bool isUint64() const = 0;
		virtual bool isFloat()  const = 0;
		virtual bool isDouble() const = 0;
		virtual bool isString() const = 0;

		virtual bool        getBool()   const = 0;
		virtual int         getInt()    const = 0;
		virtual unsigned    getUint()   const = 0;
		virtual int64_t     getInt64()  const = 0;
		virtual uint64_t    getUint64() const = 0;
		virtual float       getFloat()  const = 0;
		virtual double      getDouble() const = 0;
		virtual std::string getString() const = 0;

		virtual void setBool(bool b) = 0;
		virtual void setInt(int i) = 0;
		virtual void setUint(unsigned u) = 0;
		virtual void setInt64(int64_t i) = 0;
		virtual void setUint64(uint64_t u) = 0;
		virtual void setFloat(float f) = 0;
		virtual void setDouble(double d) = 0;
		virtual void setString(std::string_view str) = 0;

		virtual ~PrimitiveHolder() = default;
	};

	template<typename Type>
	class PrimitiveHolderImpl : public PrimitiveHolder, public ValueHolderImpl<Type> {
	public:
		PrimitiveHolderImpl(Type* ptr) : ValueHolderImpl<Type>(ptr) {}

		bool isBool()   const override { return isType<bool>(); }
		bool isInt()    const override { return isType<int>(); }
		bool isUint()   const override { return isType<unsigned>(); }
		bool isInt64()  const override { return isType<int64_t>(); }
		bool isUint64() const override { return isType<uint64_t>(); }
		bool isFloat()  const override { return isType<float>(); }
		bool isDouble() const override { return isType<double>(); }
		bool isString() const override { return isType<std::string>(); }

		bool        getBool()   const override { return GetIfMatched<bool,     ValueType>::get(value()); }
		int         getInt()    const override { return GetIfMatched<int,      ValueType>::get(value()); }
		unsigned    getUint()   const override { return GetIfMatched<unsigned, ValueType>::get(value()); }
		int64_t     getInt64()  const override { return GetIfMatched<int64_t,  ValueType>::get(value()); }
		uint64_t    getUint64() const override { return GetIfMatched<uint64_t, ValueType>::get(value()); }
		float       getFloat()  const override { return GetIfMatched<float,    ValueType>::get(value()); }
		double      getDouble() const override { return GetIfMatched<double,   ValueType>::get(value()); }
		std::string getString() const override { return GetIfMatched<std::string, ValueType>::get(value()); }

		void setBool(bool b)       override { AssignIfMatched<ValueType, bool>::assign(value(), b); }
		void setInt(int i)         override { AssignIfMatched<ValueType, int>::assign(value(), i); }
		void setUint(unsigned u)   override { AssignIfMatched<ValueType, unsigned>::assign(value(), u); }
		void setInt64(int64_t i)   override { AssignIfMatched<ValueType, int64_t>::assign(value(), i); }
		void setUint64(uint64_t u) override { AssignIfMatched<ValueType, uint64_t>::assign(value(), u); }
		void setFloat(float f)     override { AssignIfMatched<ValueType, float>::assign(value(), f); }
		void setDouble(double d)   override { AssignIfMatched<ValueType, double>::assign(value(), d); }
		void setString(std::string_view str) override { AssignIfMatched<ValueType, std::string_view>::assign(value(), str); }
	};

	class ObjectHolder : public virtual ValueHolder {
	public:
		virtual MemberIterator      memberBegin() = 0;
		virtual MemberIterator      memberEnd() = 0;
		virtual ConstMemberIterator memberBegin() const = 0;
		virtual ConstMemberIterator memberEnd()   const = 0;
	};

	template<typename Type>
	class ObjectHolderImpl : public ObjectHolder, public ValueHolderImpl<Type> {
	public:
		ObjectHolderImpl(Type* ptr, std::vector<Member> m)
			: ValueHolderImpl<Type>(ptr), members(std::move(m)) {}

		void reinit() override {
			ValueHolderImpl<Type>::reinit();
			members = buildStructMemPtrTree(*ValueHolderImpl<Type>::pointer());
		}

		void setNone() override { 
			ValueHolderImpl<Type>::setNone(); 
			members.clear(); 
		}

		MemberIterator      memberBegin()       override { return members.begin(); }
		MemberIterator      memberEnd()         override { return members.end(); }
		ConstMemberIterator memberBegin() const override { return members.cbegin(); }
		ConstMemberIterator memberEnd()   const override { return members.cend(); }

	private:
		std::vector<Member> members;
	};

	class ArrayHolder : public virtual ValueHolder {
	public:
		virtual bool isResizable() const = 0;
		virtual void resize(std::size_t newSize) = 0;
		virtual std::size_t size() const = 0;

		virtual bool canHoldNoneElem() const = 0;

		virtual ArrayIterator      arrayBegin() = 0;
		virtual ArrayIterator      arrayEnd() = 0;
		virtual ConstArrayIterator arrayBegin() const = 0;
		virtual ConstArrayIterator arrayEnd()   const = 0;
	};

	using ArrayResizer = std::function<std::vector<Value>(std::size_t)>;

	template<typename Type>
	class ArrayHolderImpl : public ArrayHolder, public ValueHolderImpl<Type> {
	public:
		ArrayHolderImpl(Type* arrayMemPtr, std::vector<Value> elements, Value::ArrayResizer r)
			: ValueHolderImpl<Type>(arrayMemPtr), elems(std::move(elements)), resizer(r) {}

		void reinit() override {
			ValueHolderImpl<Type>::reinit();
			elems = arrayToValues(*ValueHolderImpl<Type>::pointer());
		}

		void setNone() override {
			ValueHolderImpl<Type>::setNone();
			elems.clear();
		}

		bool isResizable()         const override { return resizer != nullptr; }
		void resize(std::size_t newSize) override { elems = resizer(newSize); }
		std::size_t size()         const override { return elems.size(); }

		bool canHoldNoneElem() const override { return can_hold_null_elem<ValueType>::value; }

		ArrayIterator      arrayBegin() override { return elems.begin(); }
		ArrayIterator      arrayEnd()   override { return elems.end(); }
		ConstArrayIterator arrayBegin() const override { return elems.begin(); }
		ConstArrayIterator arrayEnd()   const override { return elems.end(); }
	
	private:
		std::vector<Value> elems;
		ArrayResizer resizer = nullptr;
	};

	using ValueHolderPtr  = std::shared_ptr<ValueHolder>;
	using PrimHolderPtr   = std::shared_ptr<PrimitiveHolder>;
	using ObjectHolderPtr = std::shared_ptr<ObjectHolder>;
	using ArrayHolderPtr  = std::shared_ptr<ArrayHolder>;

	ValueHolderPtr  valuePtr() const { return castTo<ValueHolderPtr>(held); }
	PrimHolderPtr   primPtr()  const { return isPrimitive() ? castTo<PrimHolderPtr>(held)   : nullptr; }
	ObjectHolderPtr objPtr()   const { return isObject() ? castTo<ObjectHolderPtr>(held) : nullptr; }
	ArrayHolderPtr  arrayPtr() const { return isArray() ? castTo<ArrayHolderPtr>(held)  : nullptr; }

	template<typename Type>
	using EnableIfIsValidHolderPtr = std::enable_if_t<std::disjunction_v<
		std::is_same<Type, PrimHolderPtr>,
		std::is_same<Type, ObjectHolderPtr>,
		std::is_same<Type, ArrayHolderPtr>,
		std::is_same<Type, ValueHolderPtr>
		>>;

	template<typename Type, typename = EnableIfIsValidHolderPtr<Type>>
	Type castTo(const std::variant<PrimHolderPtr, ObjectHolderPtr, ArrayHolderPtr>& held) const {
		if constexpr (!std::is_same_v<Type, ValueHolderPtr>) {
			return std::get<Type>(held);
		}
		else {
			// Implicitly up-casts to ValueHolderPtr
			return std::visit([](auto ptr) -> ValueHolderPtr { return ptr; }, held);
		}
	}


	Value(const Value&) = delete;
	Value& operator=(const Value&) = delete;

	template<typename Type>
	Value(Type* primtivePtr, PrimitiveTag);

	template<typename Type>
	Value(Type* structPtr, StructTag);

	template<typename Type>
    Value(Type* seqContainerPtr, SequentialContainerTag);

	template<typename Type>
	Value(Type* seqContainerPtr, TupleTag);


	std::variant<PrimHolderPtr, ObjectHolderPtr, ArrayHolderPtr> held;
};


template<bool Const, typename ValueT>
class Primitive {
public:
	using ValueType = typename MaybeAddConst<Const, ValueT>::type;
	using PrimitiveType = Primitive<false, Value>;
	using ConstPrimitiveType = Primitive<true, Value>;

	operator ValueType&() const{ return value; }

	bool canBeNone()    const { return value.canBeNone(); }
	bool isNone()       const { return value.isNone(); }
	void setNone() { value.setNone(); }
	void reinit() { value.reinit(); }

	bool isBool()   const { return value.isBool(); }
	bool isInt()    const { return value.isInt(); }
	bool isUint()   const { return value.isUint(); }
	bool isInt64()  const { return value.isInt64(); }
	bool isUint64() const { return value.isUint64(); }
	bool isFloat()  const { return value.isFloat(); }
	bool isDouble() const { return value.isDouble(); }
	bool isString() const { return value.isString(); }

	bool        getBool()   const { return value.getBool(); }
	int         getInt()    const { return value.getInt(); }
	unsigned    getUint()   const { return value.getUint(); }
	int64_t     getInt64()  const { return value.getInt64(); }
	uint64_t    getUint64() const { return value.getUint64(); }
	float       getFloat()  const { return value.getFloat(); }
	double      getDouble() const { return value.getDouble(); }
	std::string getString() const { return value.getString(); }

	void setBool(bool b)       { value.setBool(b); }
	void setInt(int i)         { value.setInt(i); }
	void setUint(unsigned u)   { value.setUint(u); }
	void setInt64(int64_t i)   { value.setInt64(i); }
	void setUint64(uint64_t u) { value.setUint64(u); }
	void setFloat(float f)     { value.setFloat(f); }
	void setDouble(double d)   { value.setDouble(d); }
	void setString(std::string_view str) { value.setString(str); }

private:
	Primitive(ValueType& v) : value(v) {}
	friend class Value;
	ValueType& value;
};

class Value::Member {
public:
	Member(std::string_view name, Value value) :
		name_(name), value_(std::move(value)) {}

	const std::string& name() const { return name_; }

	Value& value()             { return value_; }
	const Value& value() const { return value_; }

private:
	std::string name_;
	Value value_;
};


template<bool Const, typename ValueT>
class Object {
public:
	using ValueType = typename MaybeAddConst<Const, ValueT>::type;
	using Member = typename ValueT::Member;
	using MemberIterator = std::conditional_t<Const, typename ValueT::ConstMemberIterator, 
		                                             typename ValueT::MemberIterator>;
	using ObjectType = Object<false, ValueT>;
	using ConstObjectType = Object<true, ValueT>;

	operator ValueType& () const { return value; }

	bool canBeNone()    const { return value.canBeNone(); }
	bool isNone()       const { return value.isNone(); }
	void setNone() { value.setNone(); }
	void reinit() { value.reinit(); }

	MemberIterator begin() { return value.memberBegin(); }
	MemberIterator end()   { return value.memberEnd(); }

private:
	Object(ValueType& v) : value(v) {}
	friend class Value;
	ValueType& value;
};


template<bool Const, typename ValueT>
class Array {
public:
	using ValueType = typename MaybeAddConst<Const, ValueT>::type;
	using ArrayIterator = std::conditional_t<Const, typename ValueT::ConstArrayIterator,
		                                            typename ValueT::ArrayIterator>;
	using ArrayType = Array<false, ValueT>;
	using ConstArrayType = Array<true, ValueT>;

	operator ValueType& () const { return value; }

	bool canBeNone()    const { return value.canBeNone(); }
	bool isNone()       const { return value.isNone(); }
	void setNone() { value.setNone(); }
	void reinit()  { value.reinit(); }

	bool isResizable()     const { return value.isResizable(); }
	std::size_t size()     const { return value.size(); }
	void resize(std::size_t newSize) { value.resize(newSize); }
	bool canHoldNoneElem() const { return value.canHoldNoneElem(); }

	ArrayIterator begin() { return value.arrayBegin(); }
	ArrayIterator end()   { return value.arrayEnd(); }

private:
	Array(ValueType& v) : value(v) {}
	friend class Value;
	ValueType& value;
};


template<typename Type>
inline Value::Value(Type* ptr) 
	: Value(ptr, Type2Tag<Type>::Tag{}) {
	assert(ptr != nullptr);
}

template<typename Type>
inline Value::Value(Type* primtivePtr, PrimitiveTag)
	: held(std::make_shared<PrimitiveHolderImpl<Type>>(primtivePtr)) {
	static_assert(is_jsonable_primitive_type_v<Type>, 
		          "Type must be a JSON - serializable primitive (e.g., int, double, bool, string)");
	assert(primtivePtr != nullptr);
}

template<typename Type>
inline Value::Value(Type* structPtr, StructTag) {
	static_assert(is_jsonable_struct_v<Type>,
		"Type should be JSON-serializable class or struct");
	assert(structPtr != nullptr);

	held = std::make_shared<ObjectHolderImpl<Type>>(structPtr,
			                                        buildStructMemPtrTree(*structPtr));
}

template<typename Type>
Value::Value(Type* seqContainerPtr, SequentialContainerTag) {
	static_assert(is_jsonable_sequential_container_v<Type>,
		"Type should be JSON-serializable sequential container");
	assert(seqContainerPtr != nullptr);

	constexpr bool isResizable =
		is_jsonable_dynamic_array_v<Type> && !std::is_const_v<Type>;

	Value::ArrayResizer resizer;
	if constexpr (isResizable)
		resizer = createResizerFrom(*seqContainerPtr);
	else
		resizer = nullptr;

	held = std::make_shared<ArrayHolderImpl<Type>>(seqContainerPtr,
		                                           seqToValues(*seqContainerPtr), 
		                                           resizer);
}

template<typename Type>
Value::Value(Type* tuplePtr, TupleTag) {
	static_assert(is_jsonable_tuple_v<Type>,
		"Type should be JSON-serializable tuple");

	held = std::make_shared<ArrayHolderImpl<Type>>(tuplePtr,
		                                           tupleToValues(*tuplePtr), 
		                                           nullptr);
}


inline Value::PrimitiveType Value::asPrimitive() { assert(isModifiable() && isPrimitive()); return PrimitiveType(*this); }
inline Value::ObjectType Value::asObject()       { assert(isModifiable() && isObject());    return ObjectType(*this); }
inline Value::ArrayType Value::asArray()         { assert(isModifiable() && isArray());     return ArrayType(*this); }

inline Value::ConstPrimitiveType Value::asConstPrimitive() const { assert(isPrimitive()); return ConstPrimitiveType(*this); }
inline Value::ConstObjectType Value::asConstObject()       const { assert(isObject());    return ConstObjectType(*this); }
inline Value::ConstArrayType Value::asConstArray()         const { assert(isArray());     return ConstArrayType(*this); }


template<typename Container>
auto createResizerFrom(Container& sequence) -> std::function<std::vector<Value>(std::size_t)> {
	static_assert(is_jsonable_dynamic_array_v<Container>);

	auto resizer = [&sequence](std::size_t newSize) -> std::vector<Value> {
		AccessPolicy<Container> acc(&sequence); 
		acc.value().resize(newSize);

		return seqToValues(sequence);
	};

	return resizer;
}


template<typename Array>
std::vector<Value> arrayToValues(Array& array) {
	static_assert(is_jsonable_sequential_container_v<Array> ||
		          is_jsonable_tuple_v<Array>);

	if constexpr (is_jsonable_sequential_container_v<Array>)
		return seqToValues(array);
	else
		return tupleToValues(array);
}


template<typename Container>
std::vector<Value> seqToValues(Container& sequence) {
	static_assert(is_jsonable_sequential_container_v<Container>);

	AccessPolicy<Container> acc(&sequence);
	if (acc.isNone())
		return {};


	std::vector<Value> values;
	for (auto&& item : acc.value())
		values.emplace_back(Value(&item));

	return values;
}


template<typename Tuple>
std::vector<Value> tupleToValues(Tuple& tuple) {
	static_assert(is_jsonable_tuple_v<Tuple>);

	AccessPolicy<Tuple> acc(&tuple);
	if (acc.isNone())
		return {};


	std::vector<Value> values;
	std::apply([&values](auto&&... tupleArgs)
		{
			(..., (values.emplace_back(Value(&tupleArgs))));
		},
		acc.value());

	return values;
}


template<typename Struct>
std::vector<Value::Member> buildStructMemPtrTree(Struct& s) {
	static_assert(is_jsonable_struct_v<Struct>);

	AccessPolicy<Struct> acc(&s);
	if (acc.isNone())
		return {};


	std::vector<Value::Member> members;

	auto descriptors = Descriptor<std::remove_cv_t<remove_shared_optional_t<Struct>>>::member_descriptors;
	for_each(descriptors, [&acc, &members](auto desc) {
		constexpr auto n = getMemberName(desc);
		auto& v          = getMemberValue(acc.value(), desc);

		members.emplace_back(Value::Member{n, Value(&v) });
	});

	return members;
}

template<typename Desc>
constexpr auto getMemberName(Desc descriptor) {
	return descriptor.name();
}

template<typename Struct, typename Desc>
auto& getMemberValue(Struct& s, Desc descriptor) {
	return s.*(descriptor.pointer());
}

}  // namespace detail

}  // namespace rapidjson_util 

#endif