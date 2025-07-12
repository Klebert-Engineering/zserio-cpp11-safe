#ifndef ZSERIO_ANY_HOLDER_H_INC
#define ZSERIO_ANY_HOLDER_H_INC

#include <cstddef>
#include <type_traits>

#include "zserio/AllocatorHolder.h"
#include "zserio/CppRuntimeException.h"
#include "zserio/NoInit.h"
#include "zserio/OptionalHolder.h"
#include "zserio/RebindAlloc.h"
#include "zserio/Result.h"
#include "zserio/Traits.h"
#include "zserio/Types.h"

namespace zserio
{

namespace detail
{

class TypeIdHolder
{
public:
    using type_id = const int*;

    template <typename T>
    static type_id get()
    {
        static int currentTypeId;

        return &currentTypeId;
    }
};

// Interface for object holders
template <typename ALLOC>
class IHolder
{
public:
    virtual ~IHolder() = default;
    virtual Result<IHolder*> clone(const ALLOC& allocator) const noexcept = 0;
    virtual Result<IHolder*> clone(NoInitT, const ALLOC& allocator) const noexcept = 0;
    virtual Result<IHolder*> clone(void* storage) const noexcept = 0;
    virtual Result<IHolder*> clone(NoInitT, void* storage) const noexcept = 0;
    virtual Result<IHolder*> move(const ALLOC& allocator) noexcept = 0;
    virtual Result<IHolder*> move(NoInitT, const ALLOC& allocator) noexcept = 0;
    virtual Result<IHolder*> move(void* storage) noexcept = 0;
    virtual Result<IHolder*> move(NoInitT, void* storage) noexcept = 0;
    virtual void destroy(const ALLOC& allocator) noexcept = 0;
    virtual bool isType(detail::TypeIdHolder::type_id typeId) const noexcept = 0;
};

// Base of object holders, holds a value in the inplace_optional_holder
template <typename T, typename ALLOC>
class HolderBase : public IHolder<ALLOC>
{
public:
    template <typename U>
    void set(U&& value)
    {
        m_typedHolder = std::forward<U>(value);
    }

    template <typename U, typename std::enable_if<std::is_constructible<U, NoInitT, U>::value, int>::type = 0>
    void set(NoInitT, U&& value)
    {
        // inplace_optional_holder constructor to prevent it's implicit constructor
        m_typedHolder.assign(NoInit, inplace_optional_holder<T>(NoInit, std::forward<U>(value)));
    }

    template <typename U, typename std::enable_if<!std::is_constructible<U, NoInitT, U>::value, int>::type = 0>
    void set(NoInitT, U&& value)
    {
        m_typedHolder = std::forward<U>(value);
    }

    void setHolder(const inplace_optional_holder<T>& value)
    {
        m_typedHolder = value;
    }

    void setHolder(inplace_optional_holder<T>&& value)
    {
        m_typedHolder = std::move(value);
    }

    template <typename U, typename std::enable_if<std::is_constructible<U, NoInitT, U>::value, int>::type = 0>
    void setHolder(NoInitT, const inplace_optional_holder<U>& value)
    {
        m_typedHolder.assign(NoInit, value);
    }

    template <typename U, typename std::enable_if<std::is_constructible<U, NoInitT, U>::value, int>::type = 0>
    void setHolder(NoInitT, inplace_optional_holder<U>&& value)
    {
        m_typedHolder.assign(NoInit, std::move(value));
    }

    template <typename U, typename std::enable_if<!std::is_constructible<U, NoInitT, U>::value, int>::type = 0>
    void setHolder(NoInitT, const inplace_optional_holder<U>& value)
    {
        m_typedHolder = value;
    }

    template <typename U, typename std::enable_if<!std::is_constructible<U, NoInitT, U>::value, int>::type = 0>
    void setHolder(NoInitT, inplace_optional_holder<U>&& value)
    {
        m_typedHolder = std::move(value);
    }

    T& get()
    {
        return m_typedHolder.value();
    }

    const T& get() const
    {
        return m_typedHolder.value();
    }

    bool isType(detail::TypeIdHolder::type_id typeId) const override
    {
        return detail::TypeIdHolder::get<T>() == typeId;
    }

protected:
    inplace_optional_holder<T>& getHolder()
    {
        return m_typedHolder;
    }

    const inplace_optional_holder<T>& getHolder() const
    {
        return m_typedHolder;
    }

private:
    inplace_optional_holder<T> m_typedHolder;
};

// Holder allocated on heap
template <typename T, typename ALLOC>
class HeapHolder : public HolderBase<T, ALLOC>
{
private:
    struct ConstructTag
    {};

public:
    using this_type = HeapHolder<T, ALLOC>;

    explicit HeapHolder(ConstructTag) noexcept
    {}

    static Result<this_type*> create(const ALLOC& allocator) noexcept
    {
        using AllocType = RebindAlloc<ALLOC, this_type>;
        using AllocTraits = std::allocator_traits<AllocType>;

        try
        {
            AllocType typedAlloc = allocator;
            typename AllocTraits::pointer ptr = AllocTraits::allocate(typedAlloc, 1);
            // this never throws because HeapHolder constructor never throws
            AllocTraits::construct(typedAlloc, std::addressof(*ptr), ConstructTag{});
            return Result<this_type*>::success(ptr);
        }
        catch (...)
        {
            return Result<this_type*>::error(ErrorCode::AllocationFailed);
        }
    }

    Result<IHolder<ALLOC>*> clone(const ALLOC& allocator) const noexcept override
    {
        auto holderResult = create(allocator);
        if (holderResult.isError())
        {
            return Result<IHolder<ALLOC>*>::error(holderResult.getError());
        }
        this_type* holder = holderResult.getValue();
        holder->setHolder(this->getHolder());
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> clone(NoInitT, const ALLOC& allocator) const noexcept override
    {
        auto holderResult = create(allocator);
        if (holderResult.isError())
        {
            return Result<IHolder<ALLOC>*>::error(holderResult.getError());
        }
        this_type* holder = holderResult.getValue();
        holder->setHolder(NoInit, this->getHolder());
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> clone(void*) const noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> clone(NoInitT, void*) const noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> move(const ALLOC& allocator) noexcept override
    {
        auto holderResult = create(allocator);
        if (holderResult.isError())
        {
            return Result<IHolder<ALLOC>*>::error(holderResult.getError());
        }
        this_type* holder = holderResult.getValue();
        holder->setHolder(std::move(this->getHolder()));
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> move(NoInitT, const ALLOC& allocator) noexcept override
    {
        auto holderResult = create(allocator);
        if (holderResult.isError())
        {
            return Result<IHolder<ALLOC>*>::error(holderResult.getError());
        }
        this_type* holder = holderResult.getValue();
        holder->setHolder(NoInit, std::move(this->getHolder()));
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> move(void*) noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> move(NoInitT, void*) noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    void destroy(const ALLOC& allocator) noexcept override
    {
        using AllocType = RebindAlloc<ALLOC, this_type>;
        using AllocTraits = std::allocator_traits<AllocType>;

        AllocType typedAlloc = allocator;
        AllocTraits::destroy(typedAlloc, this);
        AllocTraits::deallocate(typedAlloc, this, 1);
    }
};

// Holder allocated in the in-place storage
template <typename T, typename ALLOC>
class NonHeapHolder : public HolderBase<T, ALLOC>
{
public:
    using this_type = NonHeapHolder<T, ALLOC>;

    static this_type* create(void* storage)
    {
        return new (storage) this_type();
    }

    Result<IHolder<ALLOC>*> clone(const ALLOC&) const noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> clone(NoInitT, const ALLOC&) const noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> clone(void* storage) const noexcept override
    {
        if (storage == nullptr)
        {
            return Result<IHolder<ALLOC>*>::error(ErrorCode::NullPointer);
        }
        NonHeapHolder* holder = new (storage) NonHeapHolder();
        holder->setHolder(this->getHolder());
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> clone(NoInitT, void* storage) const noexcept override
    {
        if (storage == nullptr)
        {
            return Result<IHolder<ALLOC>*>::error(ErrorCode::NullPointer);
        }
        NonHeapHolder* holder = new (storage) NonHeapHolder();
        holder->setHolder(NoInit, this->getHolder());
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> move(const ALLOC&) noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> move(NoInitT, const ALLOC&) noexcept override
    {
        return Result<IHolder<ALLOC>*>::error(ErrorCode::InvalidOperation);
    }

    Result<IHolder<ALLOC>*> move(void* storage) noexcept override
    {
        if (storage == nullptr)
        {
            return Result<IHolder<ALLOC>*>::error(ErrorCode::NullPointer);
        }
        NonHeapHolder* holder = new (storage) NonHeapHolder();
        holder->setHolder(std::move(this->getHolder()));
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    Result<IHolder<ALLOC>*> move(NoInitT, void* storage) noexcept override
    {
        if (storage == nullptr)
        {
            return Result<IHolder<ALLOC>*>::error(ErrorCode::NullPointer);
        }
        NonHeapHolder* holder = new (storage) NonHeapHolder();
        holder->setHolder(NoInit, std::move(this->getHolder()));
        return Result<IHolder<ALLOC>*>::success(static_cast<IHolder<ALLOC>*>(holder));
    }

    void destroy(const ALLOC&) noexcept override
    {
        this->~NonHeapHolder();
    }

private:
    NonHeapHolder() = default;
};

template <typename ALLOC>
union UntypedHolder
{
    // 2 * sizeof(void*) for T + sizeof(void*) for Holder's vptr
    using MaxInPlaceType = std::aligned_storage<3 * sizeof(void*), alignof(void*)>::type;

    detail::IHolder<ALLOC>* heap;
    MaxInPlaceType inPlace;
};

template <typename T, typename ALLOC>
using has_non_heap_holder = std::integral_constant<bool,
        sizeof(NonHeapHolder<T, ALLOC>) <= sizeof(typename UntypedHolder<ALLOC>::MaxInPlaceType) &&
                std::is_nothrow_move_constructible<T>::value &&
                alignof(T) <= alignof(typename UntypedHolder<ALLOC>::MaxInPlaceType)>;

} // namespace detail

/**
 * Type safe container for single values of any type which doesn't need RTTI.
 */
template <typename ALLOC = std::allocator<uint8_t>>
class AnyHolder : public AllocatorHolder<ALLOC>
{
    using AllocTraits = std::allocator_traits<ALLOC>;
    using AllocatorHolder<ALLOC>::get_allocator_ref;
    using AllocatorHolder<ALLOC>::set_allocator;

public:
    using AllocatorHolder<ALLOC>::get_allocator;
    using allocator_type = ALLOC;

    /**
     * Empty constructor.
     */
    AnyHolder() :
            AnyHolder(ALLOC())
    {}

    /**
     * Constructor from given allocator
     */
    explicit AnyHolder(const ALLOC& allocator) :
            AllocatorHolder<ALLOC>(allocator)
    {
        m_untypedHolder.heap = nullptr;
    }

    /**
     * Constructor from any value.
     *
     * \param value Value of any type to hold. Supports move semantic.
     */
    template <typename T,
            typename std::enable_if<!std::is_same<typename std::decay<T>::type, AnyHolder>::value &&
                            !std::is_same<typename std::decay<T>::type, ALLOC>::value,
                    int>::type = 0>
    explicit AnyHolder(T&& value, const ALLOC& allocator = ALLOC()) :
            AllocatorHolder<ALLOC>(allocator)
    {
        m_untypedHolder.heap = nullptr;
        set(std::forward<T>(value));
    }

    /**
     * Constructor from any value which prevents initialization.
     *
     * \param value Value of any type to hold. Supports move semantic.
     */
    template <typename T,
            typename std::enable_if<!std::is_same<typename std::decay<T>::type, AnyHolder>::value, int>::type =
                    0>
    explicit AnyHolder(NoInitT, T&& value, const ALLOC& allocator = ALLOC()) :
            AllocatorHolder<ALLOC>(allocator)
    {
        m_untypedHolder.heap = nullptr;
        set(NoInit, std::forward<T>(value));
    }

    /**
     * Destructor.
     */
    ~AnyHolder()
    {
        clearHolder();
    }

    /**
     * Copy constructor.
     *
     * \param other Any holder to copy.
     */
    AnyHolder(const AnyHolder& other) :
            AllocatorHolder<ALLOC>(
                    AllocTraits::select_on_container_copy_construction(other.get_allocator_ref()))
    {
        copy(other);
    }

    /**
     * Copy constructor which prevents initialization.
     *
     * \param other Any holder to copy.
     */
    AnyHolder(NoInitT, const AnyHolder& other) :
            AllocatorHolder<ALLOC>(
                    AllocTraits::select_on_container_copy_construction(other.get_allocator_ref()))
    {
        copy(NoInit, other);
    }

    /**
     * Allocator-extended copy constructor.
     *
     * \param other Any holder to copy.
     * \param allocator Allocator to be used for dynamic memory allocations.
     */
    AnyHolder(const AnyHolder& other, const ALLOC& allocator) :
            AllocatorHolder<ALLOC>(allocator)
    {
        copy(other);
    }

    /**
     * Allocator-extended copy constructor which prevents initialization.
     *
     * \param other Any holder to copy.
     * \param allocator Allocator to be used for dynamic memory allocations.
     */
    AnyHolder(NoInitT, const AnyHolder& other, const ALLOC& allocator) :
            AllocatorHolder<ALLOC>(allocator)
    {
        copy(NoInit, other);
    }

    /**
     * Copy assignment operator.
     *
     * \param other Any holder to copy.
     *
     * \return Reference to this.
     */
    AnyHolder& operator=(const AnyHolder& other)
    {
        if (this != &other)
        {
            // TODO: do not dealloc unless necessary
            clearHolder();
            if (AllocTraits::propagate_on_container_copy_assignment::value)
            {
                set_allocator(other.get_allocator_ref());
            }
            copy(other);
        }

        return *this;
    }

    /**
     * Copy assignment operator which prevents initialization.
     *
     * \param other Any holder to copy.
     *
     * \return Reference to this.
     */
    AnyHolder& assign(NoInitT, const AnyHolder& other)
    {
        if (this != &other)
        {
            // TODO: do not dealloc unless necessary
            clearHolder();
            if (AllocTraits::propagate_on_container_copy_assignment::value)
            {
                set_allocator(other.get_allocator_ref());
            }
            copy(NoInit, other);
        }

        return *this;
    }

    /**
     * Move constructor.
     *
     * \param other Any holder to move from.
     */
    AnyHolder(AnyHolder&& other) noexcept :
            AllocatorHolder<ALLOC>(std::move(other.get_allocator_ref()))
    {
        move(std::move(other));
    }

    /**
     * Move constructor which prevents initialization.
     *
     * \param other Any holder to move from.
     */
    AnyHolder(NoInitT, AnyHolder&& other) noexcept :
            AllocatorHolder<ALLOC>(std::move(other.get_allocator_ref()))
    {
        move(NoInit, std::move(other));
    }

    /**
     * Allocator-extended move constructor.
     *
     * \param other Any holder to move from.
     * \param allocator Allocator to be used for dynamic memory allocations.
     */
    AnyHolder(AnyHolder&& other, const ALLOC& allocator) :
            AllocatorHolder<ALLOC>(allocator)
    {
        move(std::move(other));
    }

    /**
     * Allocator-extended move constructor which prevents initialization.
     *
     * \param other Any holder to move from.
     * \param allocator Allocator to be used for dynamic memory allocations.
     */
    AnyHolder(NoInitT, AnyHolder&& other, const ALLOC& allocator) :
            AllocatorHolder<ALLOC>(allocator)
    {
        move(NoInit, std::move(other));
    }

    /**
     * Move assignment operator.
     *
     * \param other Any holder to move from.
     *
     * \return Reference to this.
     */
    AnyHolder& operator=(AnyHolder&& other)
    {
        if (this != &other)
        {
            clearHolder();
            if (AllocTraits::propagate_on_container_move_assignment::value)
            {
                set_allocator(std::move(other.get_allocator_ref()));
            }
            move(std::move(other));
        }

        return *this;
    }

    /**
     * Move assignment operator which prevents initialization.
     *
     * \param other Any holder to move from.
     *
     * \return Reference to this.
     */
    AnyHolder& assign(NoInitT, AnyHolder&& other)
    {
        if (this != &other)
        {
            clearHolder();
            if (AllocTraits::propagate_on_container_move_assignment::value)
            {
                set_allocator(std::move(other.get_allocator_ref()));
            }
            move(NoInit, std::move(other));
        }

        return *this;
    }

    /**
     * Value assignment operator.
     *
     * \param value Any value to assign. Supports move semantic.
     *
     * \return Reference to this.
     */
    template <typename T,
            typename std::enable_if<!std::is_same<typename std::decay<T>::type, AnyHolder>::value, int>::type =
                    0>
    AnyHolder& operator=(T&& value)
    {
        set(std::forward<T>(value));

        return *this;
    }

    /**
     * Resets the holder.
     */
    void reset()
    {
        clearHolder();
    }

    /**
     * Sets any value to the holder.
     *
     * \param value Any value to set. Supports move semantic.
     * \return Result indicating success or error code on failure.
     */
    template <typename T>
    Result<void> set(T&& value) noexcept
    {
        auto holderResult = createHolder<typename std::decay<T>::type>();
        if (holderResult.isError())
        {
            return Result<void>::error(holderResult.getError());
        }
        holderResult.getValue()->set(std::forward<T>(value));
        return Result<void>::success();
    }

    /**
     * Sets any value to the holder and prevent initialization.
     *
     * \param value Any value to set. Supports move semantic.
     * \return Result indicating success or error code on failure.
     */
    template <typename T>
    Result<void> set(NoInitT, T&& value) noexcept
    {
        auto holderResult = createHolder<typename std::decay<T>::type>();
        if (holderResult.isError())
        {
            return Result<void>::error(holderResult.getError());
        }
        holderResult.getValue()->set(NoInit, std::forward<T>(value));
        return Result<void>::success();
    }

    /**
     * Gets value of the given type.
     *
     * \return Result containing reference to value of the requested type if the type matches,
     *         or error code if the type doesn't match or holder is empty.
     */
    template <typename T>
    Result<T&> get() noexcept
    {
        auto typeCheckResult = checkType<T>();
        if (typeCheckResult.isError())
        {
            return Result<T&>::error(typeCheckResult.getError());
        }
        return Result<T&>::success(getHolder<T>(detail::has_non_heap_holder<T, ALLOC>())->get());
    }

    /**
     * Gets value of the given type.
     *
     * \return Result containing const reference to value of the requested type if the type matches,
     *         or error code if the type doesn't match or holder is empty.
     */
    template <typename T>
    Result<const T&> get() const noexcept
    {
        auto typeCheckResult = checkType<T>();
        if (typeCheckResult.isError())
        {
            return Result<const T&>::error(typeCheckResult.getError());
        }
        return Result<const T&>::success(getHolder<T>(detail::has_non_heap_holder<T, ALLOC>())->get());
    }

    /**
     * Check whether the holder holds the given type.
     *
     * \return True if the stored value is of the given type, false otherwise.
     */
    template <typename T>
    bool isType() const
    {
        return hasHolder() && getUntypedHolder()->isType(detail::TypeIdHolder::get<T>());
    }

    /**
     * Checks whether the holder has any value.
     *
     * \return True if the holder has assigned any value, false otherwise.
     */
    bool hasValue() const
    {
        return hasHolder();
    }

private:
    Result<void> copy(const AnyHolder& other) noexcept
    {
        if (other.m_isInPlace)
        {
            auto cloneResult = other.getUntypedHolder()->clone(&m_untypedHolder.inPlace);
            if (cloneResult.isError())
            {
                return Result<void>::error(cloneResult.getError());
            }
            m_isInPlace = true;
        }
        else if (other.m_untypedHolder.heap != nullptr)
        {
            auto cloneResult = other.getUntypedHolder()->clone(get_allocator_ref());
            if (cloneResult.isError())
            {
                return Result<void>::error(cloneResult.getError());
            }
            m_untypedHolder.heap = cloneResult.getValue();
        }
        else
        {
            m_untypedHolder.heap = nullptr;
        }
        return Result<void>::success();
    }

    Result<void> copy(NoInitT, const AnyHolder& other) noexcept
    {
        if (other.m_isInPlace)
        {
            auto cloneResult = other.getUntypedHolder()->clone(NoInit, &m_untypedHolder.inPlace);
            if (cloneResult.isError())
            {
                return Result<void>::error(cloneResult.getError());
            }
            m_isInPlace = true;
        }
        else if (other.m_untypedHolder.heap != nullptr)
        {
            auto cloneResult = other.getUntypedHolder()->clone(NoInit, get_allocator_ref());
            if (cloneResult.isError())
            {
                return Result<void>::error(cloneResult.getError());
            }
            m_untypedHolder.heap = cloneResult.getValue();
        }
        else
        {
            m_untypedHolder.heap = nullptr;
        }
        return Result<void>::success();
    }

    void move(AnyHolder&& other)
    {
        if (other.m_isInPlace)
        {
            other.getUntypedHolder()->move(&m_untypedHolder.inPlace);
            m_isInPlace = true;
            other.clearHolder();
        }
        else if (other.m_untypedHolder.heap != nullptr)
        {
            if (get_allocator_ref() == other.get_allocator_ref())
            {
                // take over the other's storage
                m_untypedHolder.heap = other.m_untypedHolder.heap;
                other.m_untypedHolder.heap = nullptr;
            }
            else
            {
                // cannot steal the storage, allocate our own and move the holder
                m_untypedHolder.heap = other.getUntypedHolder()->move(get_allocator_ref());
                other.clearHolder();
            }
        }
        else
        {
            m_untypedHolder.heap = nullptr;
        }
    }

    void move(NoInitT, AnyHolder&& other)
    {
        if (other.m_isInPlace)
        {
            other.getUntypedHolder()->move(NoInit, &m_untypedHolder.inPlace);
            m_isInPlace = true;
            other.clearHolder();
        }
        else if (other.m_untypedHolder.heap != nullptr)
        {
            if (get_allocator_ref() == other.get_allocator_ref())
            {
                // take over the other's storage
                m_untypedHolder.heap = other.m_untypedHolder.heap;
                other.m_untypedHolder.heap = nullptr;
            }
            else
            {
                // cannot steal the storage, allocate our own and move the holder
                m_untypedHolder.heap = other.getUntypedHolder()->move(NoInit, get_allocator_ref());
                other.clearHolder();
            }
        }
        else
        {
            m_untypedHolder.heap = nullptr;
        }
    }

    void clearHolder()
    {
        if (hasHolder())
        {
            getUntypedHolder()->destroy(get_allocator_ref());
            m_isInPlace = false;
            m_untypedHolder.heap = nullptr;
        }
    }

    bool hasHolder() const
    {
        return (m_isInPlace || m_untypedHolder.heap != nullptr);
    }

    template <typename T>
    Result<detail::HolderBase<T, ALLOC>*> createHolder() noexcept
    {
        if (hasHolder())
        {
            if (getUntypedHolder()->isType(detail::TypeIdHolder::get<T>()))
            {
                return Result<detail::HolderBase<T, ALLOC>*>::success(getHolder<T>(detail::has_non_heap_holder<T, ALLOC>()));
            }

            clearHolder();
        }

        return createHolderImpl<T>(detail::has_non_heap_holder<T, ALLOC>());
    }

    template <typename T>
    Result<detail::HolderBase<T, ALLOC>*> createHolderImpl(std::true_type) noexcept
    {
        detail::NonHeapHolder<T, ALLOC>* holder =
                detail::NonHeapHolder<T, ALLOC>::create(&m_untypedHolder.inPlace);
        m_isInPlace = true;
        return Result<detail::HolderBase<T, ALLOC>*>::success(static_cast<detail::HolderBase<T, ALLOC>*>(holder));
    }

    template <typename T>
    Result<detail::HolderBase<T, ALLOC>*> createHolderImpl(std::false_type) noexcept
    {
        auto holderResult = detail::HeapHolder<T, ALLOC>::create(get_allocator_ref());
        if (holderResult.isError())
        {
            return Result<detail::HolderBase<T, ALLOC>*>::error(holderResult.getError());
        }
        detail::HeapHolder<T, ALLOC>* holder = holderResult.getValue();
        m_untypedHolder.heap = holder;
        return Result<detail::HolderBase<T, ALLOC>*>::success(static_cast<detail::HolderBase<T, ALLOC>*>(holder));
    }

    template <typename T>
    Result<void> checkType() const noexcept
    {
        if (!hasValue())
        {
            return Result<void>::error(ErrorCode::EmptyContainer);
        }
        if (!isType<T>())
        {
            return Result<void>::error(ErrorCode::TypeMismatch);
        }
        return Result<void>::success();
    }

    template <typename T>
    detail::HeapHolder<T, ALLOC>* getHeapHolder()
    {
        return static_cast<detail::HeapHolder<T, ALLOC>*>(m_untypedHolder.heap);
    }

    template <typename T>
    const detail::HeapHolder<T, ALLOC>* getHeapHolder() const
    {
        return static_cast<detail::HeapHolder<T, ALLOC>*>(m_untypedHolder.heap);
    }

    template <typename T>
    detail::NonHeapHolder<T, ALLOC>* getInplaceHolder()
    {
        return reinterpret_cast<detail::NonHeapHolder<T, ALLOC>*>(&m_untypedHolder.inPlace);
    }

    template <typename T>
    const detail::NonHeapHolder<T, ALLOC>* getInplaceHolder() const
    {
        return reinterpret_cast<const detail::NonHeapHolder<T, ALLOC>*>(&m_untypedHolder.inPlace);
    }

    template <typename T>
    detail::HolderBase<T, ALLOC>* getHolder(std::true_type)
    {
        return static_cast<detail::HolderBase<T, ALLOC>*>(getInplaceHolder<T>());
    }

    template <typename T>
    detail::HolderBase<T, ALLOC>* getHolder(std::false_type)
    {
        return static_cast<detail::HolderBase<T, ALLOC>*>(getHeapHolder<T>());
    }

    template <typename T>
    const detail::HolderBase<T, ALLOC>* getHolder(std::true_type) const
    {
        return static_cast<const detail::HolderBase<T, ALLOC>*>(getInplaceHolder<T>());
    }

    template <typename T>
    const detail::HolderBase<T, ALLOC>* getHolder(std::false_type) const
    {
        return static_cast<const detail::HolderBase<T, ALLOC>*>(getHeapHolder<T>());
    }

    detail::IHolder<ALLOC>* getUntypedHolder()
    {
        return (m_isInPlace)
                ? reinterpret_cast<detail::IHolder<ALLOC>*>(&m_untypedHolder.inPlace)
                : m_untypedHolder.heap;
    }

    const detail::IHolder<ALLOC>* getUntypedHolder() const
    {
        return (m_isInPlace)
                ? reinterpret_cast<const detail::IHolder<ALLOC>*>(&m_untypedHolder.inPlace)
                : m_untypedHolder.heap;
    }

    detail::UntypedHolder<ALLOC> m_untypedHolder;
    bool m_isInPlace = false;
};

} // namespace zserio

#endif // ifndef ZSERIO_ANY_HOLDER_H_INC
