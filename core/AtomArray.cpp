/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is [Open Source Virtual Machine.].
 *
 * The Initial Developer of the Original Code is
 * Adobe System Incorporated.
 * Portions created by the Initial Developer are Copyright (C) 2004-2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Adobe AS3 Team
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


/////////////////////////////////////////////////////////
// AtomArray object
/////////////////////////////////////////////////////////
#include "avmplus.h"

namespace avmplus
{
    using namespace MMgc;

    AtomArray::AtomArray (int initialCapacity)
    {
         m_length = 0;
         if (!initialCapacity)
         {
            m_atoms = 0;
         }
         else
         {
            if (initialCapacity < kMinCapacity)
                initialCapacity = kMinCapacity;
            GC *gc = GC::GetGC(this);
            setAtoms(gc, (Atom*) gc->Calloc(initialCapacity, sizeof(Atom), GC::kContainsPointers|GC::kZero));
         }
    }

    AtomArray::AtomArray (Atom *argv, int argc)
    {
        m_length = argc;
        if(argc > 0)
        {
            GC *gc = GC::GetGC(this);
            setAtoms(gc, (Atom*) gc->Calloc(argc, sizeof(Atom), GC::kContainsPointers|GC::kZero));
            for(int i=0; i < argc; i++) {
                AvmCore::atomWriteBarrier_ctor(gc, m_atoms, &m_atoms[i], argv[i]);
            }
        }
    }


    AtomArray::~AtomArray()
    {
        clear();
    }

    /////////////////////////////////////////////////////
    // Array AS API
    /////////////////////////////////////////////////////

    // Pop last element off the end of the array and shrink length
    Atom AtomArray::pop()
    {
        if (!m_length)
            return undefinedAtom;

        Atom retAtom = m_atoms[m_length - 1];
        setAtInternal(m_length - 1, 0); // so GC collects this item
        m_length--;
        return retAtom;
    }

    // n arguments are pushed on the array
    uint32_t AtomArray::push(Atom *args, int argc)
    {
        checkCapacity (m_length + argc);

        // slow path to trigger write barrier
        for(int i=0; i < argc; i++) {
            setAtInternal(m_length++, args[i]);
        }

        return argc;
    }

    // Reverse array elements
    void AtomArray::reverse()
    {
        if (m_length > 1)
        {
            for (uint32_t k = 0; k < (m_length >> 1); k++)
            {
                Atom temp = m_atoms[k];
                m_atoms[k] = m_atoms[m_length - k - 1];
                m_atoms[m_length - k - 1] = temp;
            }
        }
    }

    // return 0th element, shift rest down
    Atom AtomArray::shift()
    {
        if (!m_length)
            return undefinedAtom;

        Atom *arr = m_atoms;
        Atom retAtom = arr[0];
        setAtInternal(0, 0);
        moveAtoms (arr, 0, 1, m_length - 1);
        arr[m_length - 1] = 0; // clear item so GC can collect it.
        m_length--;

        return retAtom;
    }

    // insertPoint arg - place to insert
    // insertCount arg - number to insert
    // deleteCount - number to delete
    // args - #insertCount args to insert
    void AtomArray::splice(uint32_t insertPoint, uint32_t insertCount, uint32_t deleteCount, AtomArray *args, int offset)
    {
        long l_shiftAmount = (long)insertCount - (long) deleteCount; // long because result could be negative

        // Must be BEFORE arr = m_atoms since m_atoms might change
        checkCapacity (m_length + l_shiftAmount);

        Atom *arr = m_atoms;
        Atom *argsArr = args ? args->m_atoms : 0;

        if (l_shiftAmount < 0)
        {
            int numberBeingDeleted = -l_shiftAmount;

            // whack deleted items so their ref count goes down
            AvmCore::decrementAtomRegion(arr + insertPoint + insertCount, numberBeingDeleted);

            // shift elements down
            int toMove = m_length - insertPoint - deleteCount;
            moveAtoms (arr, insertPoint + insertCount, insertPoint + deleteCount, toMove);

            // clear top part for RC purposes
            VMPI_memset (arr + m_length - numberBeingDeleted, 0, numberBeingDeleted * sizeof(Atom));
        }
        else if (l_shiftAmount > 0)
        {
            moveAtoms (arr, insertPoint + l_shiftAmount, insertPoint, m_length - insertPoint);
            // clear for RC purposes
            VMPI_memset (arr + insertPoint, 0, l_shiftAmount * sizeof(Atom));
        }

        // Add the items to insert
        if (insertCount)
        {
            AvmAssert(args != 0);
            for (uint32_t i=0; i<insertCount; i++)
            {
                setAtInternal(insertPoint+i, argsArr[i+offset]);
            }
        }

        m_length += l_shiftAmount;
        return;
    }

    // insert array of arguments at front of array
    Atom AtomArray::unshift(Atom *args, int argc)
    {
        // shift elements up by argc
        // inserts args into initial spots

        checkCapacity (m_length + argc);
        Atom *arr = m_atoms;
        moveAtoms (arr, argc, 0, m_length);
        // clear moved element for RC purposes
        VMPI_memset (arr, 0, argc * sizeof(Atom));
        for(int i=0; i<argc; i++) {
            setAtInternal(i, args[i]);
        }

        m_length += argc;

        return nullObjectAtom;
    }

    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////

    void AtomArray::checkCapacity (int newLength)
    {
        AvmAssert(newLength >= 0);
        // !!@ handle case where capacity shrinks by 50% (resize buffer smaller)

        if (!m_atoms || newLength > int(capacity()))
        {
            // We oversize our buffer by 50% (is that the best algorithm?)
            int capacity = newLength + (newLength >> 2);
            if (capacity < kMinCapacity) capacity = kMinCapacity;
            GC* gc = GC::GetGC(this);
            Atom* newAtoms = (Atom*) gc->Calloc(capacity, sizeof(Atom), GC::kContainsPointers|GC::kZero);
            Atom* oldAtoms = m_atoms;
            setAtoms(gc, newAtoms);
            if(oldAtoms) {
                // use a memcpy to skip ref counting
                VMPI_memcpy(m_atoms, oldAtoms, m_length*sizeof(Atom));
                VMPI_memset(oldAtoms, 0, m_length*sizeof(Atom));
                gc->Free(oldAtoms);
            }
        }
    }

#ifdef DEBUGGER
    uint64_t AtomArray::bytesUsed() const
    {
        if (m_atoms)
            return capacity() * sizeof(Atom);
        else
            return 0;
    }
#endif

    void AtomArray::push (Atom a)
    {
        checkCapacity (m_length + 1);
        setAtInternal(m_length++, a);
    }

    void AtomArray::removeAt (uint32_t index)
    {
        AvmAssert (m_length > 0);
        AvmAssert (index < uint32_t(m_length));
        if (!m_length)
            return;

        checkCapacity (m_length - 1);

        m_length--;
        // use setAt instead of direct access for proper ref count maintenance
        setAtInternal(index, 0);
        Atom *arr = m_atoms;
        if (m_length)
        {
            // shift down entries
            moveAtoms (arr, index, index + 1, m_length - index);
        }
        arr[m_length] = 0; // clear our entry so GC can collect it
    }

    void AtomArray::insert (uint32_t index, Atom a)
    {
        AvmAssert(index <= m_length);

        checkCapacity (m_length + 1);
        m_length++;

        Atom *arr = m_atoms;
        // shift entries up by one to make room
        moveAtoms (arr, index + 1, index, m_length - index - 1);
        // this element is still in the array so don't let setAtInternal decrement its count
        arr[index] = 0;
        setAtInternal(index, a);
    }


    void AtomArray::clear()
    {
        if(m_atoms) {
            AvmCore::decrementAtomRegion(m_atoms, m_length);
            GC::GetGC(m_atoms)->Free(m_atoms);
            m_atoms = 0;
        }
        m_length = 0;
    }
}
