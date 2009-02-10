/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
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
 *   leon.sha@sun.com
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


#include "avmplus.h"

#ifdef FEATURE_NANOJIT

#include "CodegenLIR.h"
#include "../core/FrameState.h"

#if defined(WIN32) && defined(AVMPLUS_ARM)
#include <cmnintrin.h>
#endif

#if defined AVMPLUS_IA32 || defined AVMPLUS_AMD64
# define SSE2_ONLY(...) __VA_ARGS__
# define HAVE_CMOV(config) config.sse2
#else
# define SSE2_ONLY(...)
# define HAVE_CMOV(config) false
#endif

#ifdef _MSC_VER
    #if !defined (AVMPLUS_ARM)
    extern "C"
    {
	    int __cdecl _setjmp3(jmp_buf jmpbuf, int arg);
    }
    #else
    #include <setjmp.h>
    #undef setjmp
    extern "C"
    {
	    int __cdecl setjmp(jmp_buf jmpbuf);
    }
    #endif // AVMPLUS_ARM
#endif // _MSC_VER

#ifdef AVMPLUS_ARM
#ifdef _MSC_VER
#define RETURN_METHOD_PTR(_class, _method) \
return *((int*)&_method);
#else
#define RETURN_METHOD_PTR(_class, _method) \
union { \
    int (_class::*bar)(); \
    int foo[2]; \
}; \
bar = _method; \
return foo[0];
#endif

#elif defined AVMPLUS_MAC
#if !TARGET_RT_MAC_MACHO
// CodeWarrior makes us jump through some hoops
// to dereference pointer->method...
// Convert pointer->method to integer for Carbon.
#define RETURN_METHOD_PTR(_class, _method) \
int foo; \
asm("lwz %0,0(r5)" : "=r" (foo)); \
return foo;
#else
#define RETURN_METHOD_PTR(_class, _method) \
union { \
    int (_class::*bar)(); \
    intptr_t foo; \
}; \
bar = _method; \
return foo;
#endif

#else
#define RETURN_METHOD_PTR(_class, _method) \
return *((intptr_t*)&_method);
#endif

// VOID variant
#ifdef AVMPLUS_ARM
#ifdef _MSC_VER
#define RETURN_VOID_METHOD_PTR(_class, _method) \
return *((int*)&_method);
#else
#define RETURN_VOID_METHOD_PTR(_class, _method) \
union { \
    void (_class::*bar)(); \
    int foo[2]; \
}; \
bar = _method; \
return foo[1];
#endif

#elif defined AVMPLUS_MAC
#if !TARGET_RT_MAC_MACHO
// CodeWarrior makes us jump through some hoops
// to dereference pointer->method...
// Convert pointer->method to integer for Carbon.
#define RETURN_VOID_METHOD_PTR(_class, _method) \
int foo; \
asm("lwz %0,0(r5)" : "=r" (foo)); \
return foo;
#else
#define RETURN_VOID_METHOD_PTR(_class, _method) \
union { \
    void (_class::*bar)(); \
    intptr_t foo; \
}; \
bar = _method; \
return foo;
#endif

#else
#define RETURN_VOID_METHOD_PTR(_class, _method) \
return *((intptr_t*)&_method);
#endif

#ifdef PERFM
#define DOPROF
#include "../vprof/vprof.h"
#endif /* PERFM */

#ifdef AVMPLUS_64BIT
#define AVMCORE_integer			AvmCore::integer64
#define AVMCORE_integer_d		AvmCore::integer64_d
#define AVMCORE_integer_d_sse2	AvmCore::integer64_d_sse2
#define PTR_SCALE 3
#else
#define AVMCORE_integer			AvmCore::integer
#define AVMCORE_integer_d		AvmCore::integer_d
#define AVMCORE_integer_d_sse2	AvmCore::integer_d_sse2
#define PTR_SCALE 2
#endif

namespace avmplus
{
		#define PROFADDR(f) profAddr((void (DynamicProfiler::*)())(&f))
		#define COREADDR(f) coreAddr((int (AvmCore::*)())(&f))
		#define GCADDR(f) gcAddr((int (MMgc::GC::*)())(&f))
		#define ENVADDR(f) envAddr((int (MethodEnv::*)())(&f))
		#define TOPLEVELADDR(f) toplevelAddr((int (Toplevel::*)())(&f))
		#define SCRIPTADDR(f) scriptAddr((int (ScriptObject::*)())(&f))
		#define ARRAYADDR(f) arrayAddr((int (ArrayObject::*)())(&f))
		#define VECTORINTADDR(f) vectorIntAddr((int (IntVectorObject::*)())(&f))
		#define VECTORUINTADDR(f) vectorUIntAddr((int (UIntVectorObject::*)())(&f))
		#define VECTORDOUBLEADDR(f) vectorDoubleAddr((int (DoubleVectorObject::*)())(&f))
		#define VECTOROBJADDR(f) vectorObjAddr((int (ObjectVectorObject::*)())(&f))
		#define EFADDR(f)   efAddr((int (ExceptionFrame::*)())(&f))
		#define DEBUGGERADDR(f)   debuggerAddr((int (Debugger::*)())(&f))
		#define CLASSCLOSUREADDR(f)   classClosureAddr((int (ClassClosure::*)())(&f))

	#ifndef AVMPLUS_MAC
		#define FUNCADDR(addr) (uintptr)addr
	#else
		#if TARGET_RT_MAC_MACHO
			#define FUNCADDR(addr) (uintptr)addr
		#else
			#define FUNCADDR(addr) (*((uintptr*)addr))	
		#endif
	#endif

   #ifdef VTUNE
       extern void VTune_RegisterMethod(AvmCore* core, JITCodeInfo* inf);
   #endif  // VTUNE

		intptr_t coreAddr( int (AvmCore::*f)() )
		{
			RETURN_METHOD_PTR(AvmCore, f);
		}

		intptr_t  gcAddr( int (MMgc::GC::*f)() )
		{
			RETURN_METHOD_PTR(MMgc::GC, f);
		}
		
		intptr_t  envAddr( int (MethodEnv::*f)() )
		{
			RETURN_METHOD_PTR(MethodEnv, f);
		}

		intptr_t  toplevelAddr( int (Toplevel::*f)() )
		{
			RETURN_METHOD_PTR(Toplevel, f);
		}

	#ifdef DEBUGGER
		intptr_t  debuggerAddr( int (Debugger::*f)() )
		{
			RETURN_METHOD_PTR(Debugger, f);
		}		
	#endif /* DEBUGGER */

		intptr_t scriptAddr(int (ScriptObject::*f)())
		{
			RETURN_METHOD_PTR(ScriptObject, f);
		}

		intptr_t  arrayAddr(int (ArrayObject::*f)())
		{
			RETURN_METHOD_PTR(ArrayObject, f);
		}

		intptr_t vectorIntAddr(int (IntVectorObject::*f)())
		{
			RETURN_METHOD_PTR(IntVectorObject, f);
		}

		intptr_t vectorUIntAddr(int (UIntVectorObject::*f)())
		{
			RETURN_METHOD_PTR(UIntVectorObject, f);
		}

		intptr_t vectorDoubleAddr(int (DoubleVectorObject::*f)())
		{
			RETURN_METHOD_PTR(DoubleVectorObject, f);
		}

		intptr_t vectorObjAddr(int (ObjectVectorObject::*f)())
		{
			RETURN_METHOD_PTR(ObjectVectorObject, f);
		}
		intptr_t efAddr( int (ExceptionFrame::*f)() )
		{
			RETURN_METHOD_PTR(ExceptionFrame, f);
		}
		intptr_t classClosureAddr(int (ClassClosure::*f)())
		{
			RETURN_METHOD_PTR(ClassClosure, f);
		}

	using namespace MMgc;
	using namespace nanojit;

	#if defined _MSC_VER && !defined AVMPLUS_ARM
	#  define SETJMP ((uintptr)_setjmp3)
    #elif defined AVMPLUS_MAC_CARBON
	#  define SETJMP setjmpAddress
    #else
    #  define SETJMP ((uintptr)::setjmp)
	#endif // _MSC_VER

    #include "../core/vm_fops.h"

#ifdef NJ_SOFTFLOAT

    static double i2f(int32_t i) { return i; }
    static double u2f(uint32_t u) { return u; }
    static double fneg(double a) { return -a; }
    static double fadd(double a, double b) { return a + b; }
    static double fsub(double a, double b) { return a - b; }
    static double fmul(double a, double b) { return a * b; }
    static double fdiv(double a, double b) { return a / b; }
    static int feq(double a, double b) { return a == b; }
    static int flt(double a, double b) { return a <  b; }
    static int fgt(double a, double b) { return a >  b; }
    static int fle(double a, double b) { return a <= b; }
    static int fge(double a, double b) { return a >= b; }

    CSEFUNCTION(FUNCADDR(i2f), SIG1(F,I), i2f)
    CSEFUNCTION(FUNCADDR(u2f), SIG1(F,U), u2f)
    CSEFUNCTION(FUNCADDR(fneg), SIG1(F,F), fneg)
    CSEFUNCTION(FUNCADDR(fadd), SIG2(F,F,F), fadd)
    CSEFUNCTION(FUNCADDR(fsub), SIG2(F,F,F), fsub)
    CSEFUNCTION(FUNCADDR(fmul), SIG2(F,F,F), fmul)
    CSEFUNCTION(FUNCADDR(fdiv), SIG2(F,F,F), fdiv)
    CSEFUNCTION(FUNCADDR(feq), SIG2(B,F,F), feq)
    CSEFUNCTION(FUNCADDR(flt), SIG2(B,F,F), flt)
    CSEFUNCTION(FUNCADDR(fgt), SIG2(B,F,F), fgt)
    CSEFUNCTION(FUNCADDR(fle), SIG2(B,F,F), fle)
    CSEFUNCTION(FUNCADDR(fge), SIG2(B,F,F), fge)

    // replace fpu ops with function calls
    class SoftFloatFilter: public LirWriter
    {
    public:
        SoftFloatFilter(LirWriter *out) : LirWriter(out)
        {}

        LIns *hi(LIns *q) { 
            return ins1(LIR_qhi, q); 
        }
        LIns *lo(LIns *q) { 
            return ins1(LIR_qlo, q); 
        }

        LIns *split(LIns *a) {
            if (a->isQuad() && !a->isop(LIR_qjoin)) {
                // all quad-sized args must be qjoin's for soft-float
                a = ins2(LIR_qjoin, lo(a), hi(a));
            }
            return a;
        }

        LIns *split(const CallInfo *call, LInsp args[]) {
            LIns *lo = out->insCall(call, args);
            LIns *hi = out->ins1(LIR_callh, lo);
            return out->ins2(LIR_qjoin, lo, hi);
        }

        LIns *fcall1(const CallInfo *call, LIns *a) {
            LIns *args[] = { split(a) };
            return split(call, args);
        }

        LIns *fcall2(const CallInfo *call, LIns *a, LIns *b) {
            LIns *args[] = { split(b), split(a) };
            return split(call, args);
        }

        LIns *fcmp(const CallInfo *call, LIns *a, LIns *b) {
            LIns *args[] = { split(b), split(a) };
            return out->ins2(LIR_eq, out->insCall(call, args), out->insImm(1));
        }

        LIns *ins1(LOpcode op, LIns *a) {
            switch (op) {
                case LIR_i2f:
                    return fcall1(&ci_i2f, a);
                case LIR_u2f:
                    return fcall1(&ci_u2f, a);
                case LIR_fneg:
                    return fcall1(&ci_fneg, a);
                case LIR_fret:
                    return out->ins1(op, split(a));
                default:
                    return out->ins1(op, a);
            }
        }

        LIns *ins2(LOpcode op, LIns *a, LIns *b) {
            switch (op) {
                case LIR_fadd:
                    return fcall2(&ci_fadd, a, b);
                case LIR_fsub:
                    return fcall2(&ci_fsub, a, b);
                case LIR_fmul:
                    return fcall2(&ci_fmul, a, b);
                case LIR_fdiv:
                    return fcall2(&ci_fdiv, a, b);
                case LIR_feq:
                    return fcmp(&ci_feq, a, b);
                case LIR_flt:
                    return fcmp(&ci_flt, a, b);
                case LIR_fgt:
                    return fcmp(&ci_fgt, a, b);
                case LIR_fle:
                    return fcmp(&ci_fle, a, b);
                case LIR_fge:
                    return fcmp(&ci_fge, a, b);
            }
            return out->ins2(op, a, b);
        }

        LIns *insCall(const CallInfo *ci, LInsp args[]) {
            uint32_t argt = ci->_argtypes;

            for (uint32_t i=0, argsizes = argt>>ARGSIZE_SHIFT; argsizes != 0; i++, argsizes >>= ARGSIZE_SHIFT) {
                args[i] = split(args[i]);
            }

            if ((argt & ARGSIZE_MASK) == ARGSIZE_F) {
                // this function returns a double as two 32bit values, so replace
                // call with qjoin(qhi(call), call)
                return split(ci, args);
            } else {
                return out->insCall(ci, args);
            }
        }
    };
#endif // NJ_SOFTFLOAT

	/**
 	 * ---------------------------------
	 * Instruction convenience functions 
	 * ---------------------------------
	 */

	// address calc instruction
	LIns* CodegenLIR::leaIns(int32_t disp, LIns* base) {
        return lirout->ins2(LIR_addp, base, InsConstPtr((void*)disp));
	}

    // call 
	LIns* CodegenLIR::callIns(const CallInfo *ci, uint32_t argc, ...)
	{
        AvmAssert(argc <= MAXARGS);
        AvmAssert(argc == ci->count_args());

        LInsp args[MAXARGS];
        va_list ap;
        va_start(ap, argc);
        for (uint32_t i=0; i < argc; i++)
            args[argc-i-1] = va_arg(ap, LIns*);
        va_end(ap);

		return lirout->insCall(ci, args);
	}

    LIns *CodegenLIR::localCopy(int i) {
		Value &v = state->value(i);
		switch (bt(v.traits)) {
		case BUILTIN_number:
			return v.ins = lirout->insLoad(LIR_ldq, vars, i*8);
		case BUILTIN_boolean:
		case BUILTIN_int:
		case BUILTIN_uint:
			return v.ins = lirout->insLoad(LIR_ld, vars, i*8);
		default:
			return v.ins = lirout->insLoad(LIR_ldp, vars, i*8);
		}
    }

	LIns* CodegenLIR::localGet(int i) {
        Value& v = state->value(i);
        NanoAssert(v.traits == INT_TYPE || v.traits == UINT_TYPE || v.traits == BOOLEAN_TYPE);
		LIns *r = v.ins = lirout->insLoad(LIR_ld, vars, i*8);
		NanoAssert(!r->isQuad());
		return r;
	}

	LIns* CodegenLIR::localGetq(int i) {
        Value& v = state->value(i);
        NanoAssert(v.traits == NUMBER_TYPE);
        LIns *r = v.ins = lirout->insLoad(LIR_ldq, vars, i*8);
		NanoAssert(r->isQuad());
		return r;
	}

	// load a pointer-sized var
	LIns* CodegenLIR::localGetp(int i) {
        Value& v = state->value(i);
        NanoAssert(v.traits != NUMBER_TYPE && v.traits != INT_TYPE &&
				   v.traits != UINT_TYPE && v.traits != UINT_TYPE);
        LIns *r = v.ins = lirout->insLoad(LIR_ldp, vars, i*8);
		NanoAssert(r->isPtr());
		return r;
	}

	void CodegenLIR::localSet(int i, LIns* o)	
	{
        Value &v = state->value(i);
        v.ins = o;
        lirout->store(o, vars, i*8);
        DEBUGGER_ONLY(
            if (core->debugger() && int(i) < state->verifier->local_count) {
                lirout->store(InsConstPtr(v.traits), varTraits, i*sizeof(Traits*));
			}
        )
	}

	LIns* CodegenLIR::atomToNativeRep(int i, LIns* atom)
	{
		return atomToNativeRep(state->value(i).traits, atom);
	}

	LIns* CodegenLIR::ptrToNativeRep(Traits*t, LIns* ptr)
	{
		if (t->isMachineType())
		{
			return binaryIns(LIR_pior, ptr, InsConstAtom(kObjectType));
		}
		return ptr;
	}

#ifdef _DEBUG
	bool CodegenLIR::isPointer(int i)	{
		return !state->value(i).traits->isMachineType();
	}
#endif

	LIns* CodegenLIR::loadAtomRep(int i)
	{
		Value& v = state->value(i);
		Traits* t = v.traits;

        // handle Number first in case we need to do a quad load
		if (t == NUMBER_TYPE)
		{
			return callIns(FUNCTIONID(doubleToAtom), 2, coreAddr, localGetq(i));
		}

		if (!t || t == OBJECT_TYPE || t == VOID_TYPE)
		{
			// already Atom
			return localGetp(i);
		}

		LIns* native = localCopy(i);
		
		// short circuit immediates
		if (native->isconst())
		{
			if (t == INT_TYPE)
			{
				Atom a = core->intToAtom(int(native->constval()));
				if(AvmCore::isInteger(a))
					return InsConstAtom(a);
			}

			if (t == UINT_TYPE)
			{
				Atom a = core->uintToAtom(native->constval());
				if(AvmCore::isInteger(a))
					return InsConstAtom(a);
			}

			if (t == BOOLEAN_TYPE)
			{
				return InsConstAtom(native->constval() ? trueAtom : falseAtom);
			}
		}

		if (native->isconstp()) {
			if (!t->isMachineType() && native->constvalp() == 0) {
				return InsConstAtom(nullObjectAtom);
			}
		}

		if (t == INT_TYPE)
		{
			return callIns(FUNCTIONID(intToAtom), 2,
				coreAddr, native);
		}
		if (t == UINT_TYPE)
		{
			return callIns(FUNCTIONID(uintToAtom), 2,
				coreAddr, native);
		}
		if (t == BOOLEAN_TYPE)
		{
			LIns* i1 = binaryIns(LIR_lsh, native, InsConst(3));
			return u2p(binaryIns(LIR_or, i1, InsConst(kBooleanType)));
		}

		// possibly null pointers
		if (t == STRING_TYPE)
		{
			return binaryIns(LIR_pior, native, InsConstAtom(kStringType));
		}
		if (t == NAMESPACE_TYPE)
		{
			return binaryIns(LIR_pior, native, InsConstAtom(kNamespaceType));
		}

		return binaryIns(LIR_pior, native, InsConstAtom(kObjectType));
	}

	LIns* CodegenLIR::storeAtomArgs(int count, int index)
	{
		LIns* ap = InsAlloc(sizeof(Atom)*count);
		for (int i=0; i < count && !outOMem(); i++)
			storeIns(loadAtomRep(index++), i * sizeof(Atom), ap);
		return ap;
	}

	LIns* CodegenLIR::storeAtomArgs(LIns* receiver, int count, int index)
	{
		#ifdef AVMPLUS_VERBOSE
		if (verbose())
			core->console << "        	store args\n";
		#endif
		LIns* ap = InsAlloc(sizeof(Atom)*(count+1));
		storeIns(receiver, 0, ap);
		for (int i=1; i <= count && !outOMem(); i++)
		{
			LIns* v = loadAtomRep(index++);
			storeIns(v, sizeof(Atom)*i, ap);
		}
		return ap;
	}

    void initCodePages(PoolObject *pool) {
        if (!pool->codePages) {
            AvmCore *core = pool->core;
            GC *gc = core->gc;
			PageMgr *mgr = pool->codePages = new (gc) PageMgr();
            mgr->frago = new (gc) Fragmento(core, 24/*16mb*/);
			verbose_only(
                mgr->frago->assm()->_verbose = pool->verbose;
                if (pool->verbose) {
				    LabelMap *labels = mgr->frago->labels = new (gc) LabelMap(core, 0);
				    labels->add(core, sizeof(AvmCore), 0, "core");
                    labels->add(&core->codeContextAtom, sizeof(CodeContextAtom), 0, "codeContextAtom");
                }
			)
        }
    }

	CodegenLIR::CodegenLIR(MethodInfo* i)
		: gc(i->pool->core->gc), core(i->pool->core), pool(i->pool), info(i), patches(gc), 
		  interruptable(true)
#ifdef VTUNE
           , jitInfoList(i->core()->gc)
           , jitPendingRecords(i->core()->gc)
#endif
	{
		state = NULL;

		#ifdef AVMPLUS_MAC_CARBON
		setjmpInit();
		#endif

		abcStart = NULL;
		abcEnd   = NULL;

		overflow = false;
	
		#ifdef VTUNE
		hasDebugInfo = false;
       #endif /* VTUNE */

        initCodePages(pool);
	}

	CodegenLIR::~CodegenLIR()
	{
	}

	void CodegenLIR::clearMIRBuffers()
	{
		// free scratch buffers explicitly.  None of these have any pointers
		// in them, and none of them will have escaped into other objects.
		// freeing them now reduces GC pressure at startup time.

		// return the buffer back to the global list
		if (frag) 
			frag->releaseLirBuffer();
	}

	bool CodegenLIR::outOMem()
	{
		overflow = frag->lirbuf->outOMem();  // OOM mean overflowed the buffer 
		return overflow;
	}

	#ifdef AVMPLUS_MAC_CARBON
	int CodegenLIR::setjmpAddress = 0;

	extern "C" int __setjmp();
	
	asm int CodegenLIR::setjmpDummy(jmp_buf buf)
	{
		b __setjmp;	
	}
	
	void CodegenLIR::setjmpInit()
	{
		// CodeWarrior defies all reasonable efforts to get
		// the address of __vec_setjmp.  So, we resort to
		// a crude hack: We'll search the actual code
		// of setjmpDummy for the branch instruction.
		if (setjmpAddress == 0)
		{
			setjmpAddress = *((int*)&setjmpDummy);
		}
	}
	#endif

	LIns* CodegenLIR::atomToNativeRep(Traits* t, LIns* atom)
	{
		if (!t || t == OBJECT_TYPE || t == VOID_TYPE)
		{
			return atom;
		}
		else if (t == NUMBER_TYPE)
		{
			if (atom->isconst()) {
				Atom a = atom->constval();
				if (AvmCore::isDouble(a)) {
					return loadIns(LIR_ldqc, 0, InsConstAtom(a&~7));
				} else {
					AvmAssert(AvmCore::isInteger(a));
					return i2dIns(InsConst(int32_t(a>>3)));
				}
			} else {
				return callIns(FUNCTIONID(number_d), 1, atom);
			}
		}
		else if (t == INT_TYPE)
		{
			if (atom->isconst())
				return InsConst(AvmCore::integer_i(atom->constval()));
			else
				return callIns(FUNCTIONID(integer_i), 1, atom);
		}
		else if (t == UINT_TYPE)
		{
			if (atom->isconst())
				return InsConst(AvmCore::integer_u(atom->constval()));
			else
				return callIns(FUNCTIONID(integer_u), 1, atom);
		}
		else if (t == BOOLEAN_TYPE)
		{
			if (atom->isconst())
				return InsConst(urshift(atom->constval(),3));
			else
				return p2i(binaryIns(LIR_pursh, atom, InsConst(3)));
		}
		else
		{
			// pointer type
			if (atom->isconst())
				return InsConstAtom(atom->constval() & ~7);
			else
				return binaryIns(LIR_piand, atom, InsConstAtom(~7));
		}
		
#ifdef __GNUC__
		return 0;	// satisfy GCC, although we should never get here
#endif
	}

#ifdef _DEBUG
    class ValidateWriter: public LirWriter
    {
    public:
        ValidateWriter(LirWriter *out) : LirWriter(out)
        {}
        LIns *ins0(LOpcode op) {
            switch (op) {
                case LIR_tramp: AvmAssert(false); break;
                case LIR_neartramp: AvmAssert(false); break;
                case LIR_skip: AvmAssert(false); break;
                case LIR_nearskip: AvmAssert(false); break;
                case LIR_label: break;
                case LIR_start: break;
                default:AvmAssert(false);
            }
            return out->ins0(op);
        }

        LIns *insParam(int32_t i, int32_t kind) {
            return out->insParam(i, kind);
        }

        LIns *insImm(int32_t i) {
            return out->insImm(i);
        }

        LIns *insImmq(uint64_t i) {
            return out->insImmq(i);
        }

        LIns *ins1(LOpcode op, LIns *a) {
            switch (op) {
                case LIR_fneg:
                case LIR_fret:
				case LIR_qlo:
				case LIR_qhi:
					AvmAssert(a->isQuad());
					break;
                case LIR_not:
                case LIR_neg:
                case LIR_i2f: case LIR_u2f:
                case LIR_i2q: case LIR_u2q:
					AvmAssert(!a->isQuad());
					break;
				case LIR_ret:
					AvmAssert(a->isPtr());
					break;
                case LIR_live:
					break;
                case LIR_callh:
					AvmAssert(a->isop(LIR_icall));
					break;
                default:
					AvmAssert(false);
            }
            return out->ins1(op, a);
        }

        LIns *ins2(LOpcode op, LIns *a, LIns *b) {
            switch (op) {
                case LIR_fadd:
                case LIR_fsub:
                case LIR_fmul:
                case LIR_fdiv:
                case LIR_feq:
                case LIR_flt:
                case LIR_fgt:
                case LIR_fle:
                case LIR_fge:
				case LIR_qaddp:
				case LIR_qior:
                case LIR_qxor:
				case LIR_qiand:
				case LIR_qiadd:
				case LIR_qeq:
				case LIR_qlt:	case LIR_qult:
				case LIR_qgt:	case LIR_qugt:
				case LIR_qle:	case LIR_qule:
				case LIR_qge:	case LIR_quge:
					AvmAssert(a->isQuad() && b->isQuad());
					break;
                case LIR_qjoin:
				case LIR_add:
				case LIR_iaddp:
				case LIR_sub:
				case LIR_mul:
				case LIR_and:
				case LIR_or:
				case LIR_xor:
				case LIR_lsh:
				case LIR_rsh:
				case LIR_ush:
				case LIR_eq:
				case LIR_lt:	case LIR_ult:
				case LIR_gt:	case LIR_ugt:
				case LIR_le:	case LIR_ule:
				case LIR_ge:	case LIR_uge:
					AvmAssert(!a->isQuad() && !b->isQuad());
					break;
				case LIR_2:
					AvmAssert(a->isQuad() == b->isQuad());
					break;
                case LIR_cmov:
					AvmAssert(a->isCond() && b->isop(LIR_2) && !b->oprnd1()->isQuad() && !b->oprnd2()->isQuad()); 
					break;
                case LIR_qcmov:
					AvmAssert(a->isCond() && b->isop(LIR_2) && b->oprnd1()->isQuad() && b->oprnd2()->isQuad()); 
					break;
				case LIR_qilsh:
                case LIR_qirsh:
                case LIR_qursh:
					AvmAssert(a->isQuad() && !b->isQuad());
					break;
                default:
					AvmAssert(false);
					break;
            }
            return out->ins2(op, a, b);
        }

        LIns *insLoad(LOpcode op, LIns *base, LIns *disp) {
            switch (op) {
                case LIR_ld:
                case LIR_ldq:
                case LIR_ldc:
                case LIR_ldqc:
					AvmAssert(base->isPtr() && !disp->isQuad());
					break;
                default:
					AvmAssert(false);
					break;
            }
            return out->insLoad(op, base, disp);
        }

        LIns *insStore(LIns *value, LIns *base, LIns *disp) {
            AvmAssert(base && value && base->isPtr() && !disp->isQuad());
            return out->insStore(value, base, disp);
        }

        LIns *insStorei(LIns *value, LIns *base, int32_t d) {
            AvmAssert(base && value && base->isPtr());
            return out->insStorei(value, base, d);
        }

        LIns *insBranch(LOpcode op, LIns *cond, LIns *to) {
            switch (op) {
                case LIR_jt: AvmAssert(cond->isCond() && (!to || to->isop(LIR_label))); break;
                case LIR_jf: AvmAssert(cond->isCond() && (!to || to->isop(LIR_label))); break;
                case LIR_j:  AvmAssert(!cond && (!to || to->isop(LIR_label))); break;
                case LIR_ji: AvmAssert(!cond && to && !to->isop(LIR_label)); break;
                default: AvmAssert(false);
            }
            return out->insBranch(op, cond, to);
        }

        LIns *insGuard(LOpcode v, LIns *cond, SideExit *x) {
            AvmAssert(false);
            return out->insGuard(v, cond, x);
        }

        LIns *insAlloc(int32_t size) {
            AvmAssert(size >= 4 && isU16((size+3)>>2));
            return out->insAlloc(size);
        }

        LIns *insCall(const CallInfo *call, LInsp args[]) {
            uint32_t argt = call->_argtypes;
            for (uint32_t i = 0; i < MAXARGS; i++) {
                argt >>= ARGSIZE_SHIFT;
                ArgSize sz = ArgSize(argt & ARGSIZE_MASK);
                if (sz == ARGSIZE_NONE)
                    break;
				AvmAssert((sz == ARGSIZE_I || sz == ARGSIZE_U || sz == ARGSIZE_Q || sz == ARGSIZE_F) &&
						  ((sz == ARGSIZE_I || sz == ARGSIZE_U) == !args[i]->isQuad()));
            }
            return out->insCall(call, args);
        }
    };
#endif //  _DEBUG

// if DEFER_STORES is defined, we defer local variable stores to the
// ends of basic blocks, then only emit the ones that are live
// #define DEFER_STORES(...) __VA_ARGS__
#define DEFER_STORES(...) 

    class CopyPropagation: public LirWriter
    {
		AvmCore* core;
        GC *gc;
        LInsp *tracker; // we use WB() macro, so no DWB() here.
        LIns *vars;
        int nvar;
        BitSet dirty;
        bool hasExceptions;
    public:
        CopyPropagation(AvmCore* core, GC *gc, LirWriter *out, int nvar, bool ex) 
            : LirWriter(out), core(core), gc(gc), nvar(nvar), dirty(gc,nvar), hasExceptions(ex)
        {
            LInsp *a = (LInsp *) gc->Alloc(nvar*sizeof(LInsp), GC::kZero);
            WB(gc, this, &tracker, a);
        }

        ~CopyPropagation() {
            gc->Free(tracker);
        }

        void init(LIns *vars) {
            this->vars = vars;
        }

        void saveState() {
            LIns *vars = this->vars;
            for (int i=0, n=nvar; i < n; i++) {
                LIns *v = tracker[i];
                if (!v) 
                    continue;
                if (dirty.get(i)) {
                    if (v->isLoad() && v->oprnd1() == vars && v->oprnd2()->isconstval(i*sizeof(double))) {
                        // not modified
                        continue;
                    }
                    out->store(v, vars, i*sizeof(double));
                    dirty.clear(i);
                }
            }
        }

        void clearState() {
            VMPI_memset(tracker, 0, nvar*sizeof(LInsp));
            dirty.reset();
        }

        void trackStore(LIns *value, int d) {
            AvmAssert((d&7) == 0);
            int i = d >> 3;
            tracker[i] = value;
            dirty.set(gc, i);
        }

        void trackMerge(int i, LIns *cur, LIns *target) {
            (void) i; (void) cur; (void) target;
            /*if (cur != target) {
                tracker[i] = 0;
                dirty.clear(i);
            }*/
        }

        LIns *insLoad(LOpcode op, LIns *base, LIns *disp) {
            if (base == vars) {
                int d = disp->constval();
                AvmAssert((d&7) == 0);
                int i = d >> 3;
                LIns *val = tracker[i];
				if (!val) {
					val = out->insLoad(op, base, disp);
					tracker[i] = val;
				}
				return val;
            }
            return out->insLoad(op, base, disp);
        }

        LIns *insStore(LIns *value, LIns *base, LIns *disp) {
            if (base == vars) {
                trackStore(value, disp->constval());
                DEFER_STORES(return 0;)
            }
            return out->insStore(value, base, disp);
        }

        LIns *insStorei(LIns *value, LIns *base, int32_t d) {
            if (base == vars) {
                trackStore(value, d);
                DEFER_STORES(return 0;)
            }
            return out->insStorei(value, base, d);
        }

        LIns *ins0(LOpcode op) {
            if (op == LIR_label) {
                DEFER_STORES(saveState();)
                clearState();
            }
            return out->ins0(op);
        }

        LIns *insBranch(LOpcode v, LInsp cond, LInsp to) {
            DEFER_STORES(saveState();)
            return out->insBranch(v, cond, to);
        }

        LIns *insCall(const CallInfo *call, LInsp args[]) {
            #ifdef DEBUGGER
			if (core->debugger())
			{
                DEFER_STORES(
                    if (!call->_cse)
                        saveState();
                )
                LIns *i = out->insCall(call, args);
                // debugger might have modified locals, so make sure we reload after call.
                if (!call->_cse)
                    clearState();
                return i;
			}
            #endif

			DEFER_STORES(
				if (hasExceptions && !call->_cse)
					saveState();
			)
			return out->insCall(call, args);
        }

        // TODO
        // * reset when vars passed as arg to a function that could modify vars (debugenter/exit)
        // * handle load/store size mismatches -- loading high word of quad, etc
        // * suppress stores until ends of blocks
    };

    class CfgCseFilter: public CseFilter
    {
    public:
        CfgCseFilter(LirWriter *out, GC *gc) : CseFilter(out, gc)
        {
        }

        LIns *ins0(LOpcode op) {
            if (op == LIR_label) {
                exprs.clear();
            }
            return out->ins0(op);
        }
    };

	void emitStart(LirBuffer *lirbuf, LirWriter* &lirout, bool &overflow) {
        debug_only(
            // catch problems before they hit the buffer
			GC *gc = lirbuf->_frago->core()->gc;
            lirout = new (gc) ValidateWriter(lirout);
        )
		if (lirbuf->outOMem())
			overflow = true;
		else
		{
			lirout->ins0(LIR_start);
			// create params for saved regs -- processor specific
			for (int i=0; i < NumSavedRegs; i++) {
				LIns *p = lirout->insParam(i, 1); (void) p;
				verbose_only(if (lirbuf->names)
					lirbuf->names->addName(p, regNames[Assembler::savedRegs[i]]);)
			}
		}
	}

#ifdef AVMPLUS_VERBOSE
    VerboseWriter *pushVerboseWriter(GC *gc, LirWriter *lirout, LirBuffer *lirbuf) {
        lirbuf->names = new (gc) LirNameMap(gc, lirbuf->_frago->labels);
        return new (gc) VerboseWriter(gc, lirout, lirbuf->names);
    }
#endif

    /**
     * Specializer holds specializations of certian calls into inline code sequences.
     * this could just as easily be a standalone filter instead of subclassing
     * ExprFilter, however having one less pipeline stage saves 5% of verify
     * time for esc (2000 methods).  when/if this subclassing becomes painful
     * then a separate stage is waranted.
     */
    class Specializer: public ExprFilter
    {
        Config &config;
    public:
        Specializer(LirWriter *out, Config &config) : ExprFilter(out), config(config)
        {}

        bool isPromote(LOpcode op) {
            return (op & ~1) == LIR_i2f;
        }

        LIns *insCall(const CallInfo *call, LInsp args[]) {
            if (call == FUNCTIONID(integer_d)) {
                LIns *v = args[0];
                LOpcode op = v->opcode();
                if (isPromote(op))
                    return v->oprnd1();
                if (op == LIR_fadd || op == LIR_fsub || op == LIR_fmul) {
                    LIns *a = v->oprnd1();
                    LIns *b = v->oprnd2();
                    if (isPromote(a->opcode()) && isPromote(b->opcode()))
                        return out->ins2(LOpcode(op & ~LIR64), a->oprnd1(), b->oprnd1());
                }
				else if (op == LIR_quad) {
					// const fold
					return insImm(AvmCore::integer_d(v->constvalf()));
				}
            }

            SSE2_ONLY(if(config.sse2) {
                if (call == FUNCTIONID(integer_d))
                    call = FUNCTIONID(integer_d_sse2);
                else if (call == FUNCTIONID(doubleToAtom))
                    call = FUNCTIONID(doubleToAtom_sse2);
            })

            return out->insCall(call, args);
        }
    };

	// f(env, argc, instance, argv)
	bool CodegenLIR::prologue(FrameState* state)
	{
		this->state = state;
		abcStart = state->verifier->code_pos;
		abcEnd   = abcStart + state->verifier->code_length;
        framesize = state->verifier->frameSize;

        Fragmento *frago = pool->codePages->frago;
        frag = new (gc) Fragment(abcStart);
        LirBuffer *lirbuf = frag->lirbuf = new (gc) LirBuffer(frago);
        lirbuf->abi = ABI_CDECL;
        lirout = new (gc) LirBufWriter(lirbuf);
		verbose_only(if (core->config.bbgraph) {
			lirout = frag->cfg = new (gc) BlockLocator(gc, lirout);
		})
        debug_only(
            lirout = new (gc) ValidateWriter(lirout);
        )
        verbose_only(
			vbWriter = 0;
			if (verbose()) {
				lirout = vbWriter = pushVerboseWriter(gc, lirout, lirbuf);
			}
		)
        #ifdef NJ_SOFTFLOAT
        lirout = new (gc) SoftFloatFilter(lirout);
        #endif
        LoadFilter *loadfilter = 0;
        if (core->config.cseopt) {
            loadfilter = new (gc) LoadFilter(lirout, gc);
            lirout = new (gc) CfgCseFilter(loadfilter, gc);
        }
        lirout = new (gc) Specializer(lirout, core->config);
        CopyPropagation *copier = new (gc) CopyPropagation(core, gc, lirout,
            framesize, info->hasExceptions() != 0);
        lirout = this->copier = copier;
		
		emitStart(lirbuf, lirout, overflow);

		if (overflow)
			return false;

		// last pc value that we generated a store for 
		lastPcSave = 0;

		//
		// generate lir to define incoming method arguments.  Stack 
		// frame allocations follow.
		//

        env_param = lirout->insParam(0, 0);
        argc_param = lirout->insParam(1, 0);
	#ifdef AVMPLUS_64BIT
		argc_param = lirout->ins1(LIR_qlo, argc_param);
	#endif
        ap_param = lirout->insParam(2, 0);
        vars = InsAlloc(framesize * 8);
        lirbuf->sp = vars;
        if (loadfilter)
            loadfilter->sp = vars;
        copier->init(vars);

        verbose_only( if (lirbuf->names) {
            lirbuf->names->addName(env_param, "env");
            lirbuf->names->addName(argc_param, "argc");
            lirbuf->names->addName(ap_param, "ap");
            lirbuf->names->addName(vars, "vars");
        })

		// stack overflow check - use vars address as comparison
		if (core->minstack) {
			LIns *c = binaryIns(LIR_pult, vars, InsConstPtr((void*)core->minstack));
			LIns *b = branchIns(LIR_jf, c);
			callIns(FUNCTIONID(stkover), 1, env_param);
			LIns *label = Ins(LIR_label);
			verbose_only( if (lirbuf->names) { lirbuf->names->addName(label, "begin");	})
			b->target(label);
		}

		coreAddr = InsConstPtr(core);
		undefConst = InsConstAtom(undefinedAtom);

        #ifdef DEBUGGER
		if (core->debugger())
		{
			// pointers to traits so that the debugger can decode the locals
			// IMPORTANT don't move this around unless you change MethodInfo::boxLocals()
			varTraits = InsAlloc(state->verifier->local_count * sizeof(Traits*));
			varPtrs = InsAlloc((state->verifier->local_count + state->verifier->max_scope) * sizeof(void*));
			verbose_only( if (lirbuf->names) {
				lirbuf->names->addName(varTraits, "varTraits");
				lirbuf->names->addName(varPtrs, "varPtrs");
			})
			// in LIR the variables never move around, so initialize varPtrs once and we're done.
			for (int i=0; i < state->verifier->max_scope && !outOMem(); i++) {
				storeIns(leaIns(i*sizeof(double), vars), i*sizeof(void*), varPtrs);
			}
		}
		#endif

		// whether this sequence is interruptable or not.
		interruptable = (info->flags & AbstractFunction::NON_INTERRUPTABLE) ? false : true;
		
		// then space for the exception frame, be safe if its an init stub
		if (info->isFlagSet(AbstractFunction::HAS_EXCEPTIONS))
		{
			// [_save_eip][ExceptionFrame]
			// offsets of local vars, rel to current ESP
			_save_eip = InsAlloc(sizeof(intptr_t));
			_ef       = InsAlloc(sizeof(ExceptionFrame));
            verbose_only( if (lirbuf->names) {
                lirbuf->names->addName(_save_eip, "_save_eip");
                lirbuf->names->addName(_ef, "_ef");
            })
		}

		#ifdef DEBUGGER
		if (core->debugger())
		{
			// Allocate space for the call stack
			csn = InsAlloc(sizeof(CallStackNode));
			verbose_only( if (lirbuf->names) {
				lirbuf->names->addName(csn, "csn");
			})
		}
		#endif

		if (info->setsDxns())
		{
			dxns = InsAlloc(sizeof(Namespace*));
            verbose_only( if (lirbuf->names) {
                lirbuf->names->addName(dxns, "dxns");
            })

			// dxns = env->vtable->scope->defaultXmlNamespace
			LIns* declVTable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env_param);
			LIns* scope = loadIns(LIR_ldcp, offsetof(VTable, scope), declVTable);
			LIns* capturedDxns = loadIns(LIR_ldcp, offsetof(ScopeChain, defaultXmlNamespace), scope);
			storeIns(capturedDxns, 0, dxns);

			// dxnsSave = AvmCore::dxnsAddr
			dxnsAddrSave = loadIns(LIR_ldcp, offsetof(AvmCore, dxnsAddr), coreAddr);
		}

		for (int i=0, n = state->verifier->stackBase+state->stackDepth; i < n; i++)
		{
			Value& v = state->value(i);
			v.ins = 0;
		}

		//
		// copy args to local frame
		//

		// copy required args, and initialize optional args.
		// this whole section only applies to functions that actually
		// have arguments.

		LIns* apArg = ap_param;
		if (info->flags & AbstractFunction::HAS_OPTIONAL)
		{
			// compute offset of first optional arg
			int offset = 0;
			for (int i=0, n=info->param_count-info->optional_count; i <= n && !outOMem(); i++) {
                offset += info->paramTraits(i) == NUMBER_TYPE ? sizeof(double) : sizeof(Atom);
			}

			// now copy the default optional values
			LIns* argcarg = argc_param;
			for (int i=0, n=info->optional_count; i < n && !outOMem(); i++)
			{
				// first set the local[p+1] = defaultvalue
				int param = i + info->param_count - info->optional_count; // 0..N
				int loc = param+1;

				LIns* defaultVal = InsConstAtom(info->getDefaultValue(i));
				defaultVal = atomToNativeRep(loc, defaultVal);
                localSet(loc, defaultVal);
				
				// then generate: if (argc > p) local[p+1] = arg[p+1]
				LIns* cmp = binaryIns(LIR_le, argcarg, InsConst(param));
				LIns* br = branchIns(LIR_jt, cmp); // will patch
				copyParam(loc, offset);

                LIns *label = Ins(LIR_label);
				verbose_only( if (lirbuf->names) {
					char str[64];
					VMPI_sprintf(str,"param_%d",i);
					lirbuf->names->addName(label,str);
				})
				br->target(label);
			}
		}
		else
		{
			// !HAS_OPTIONAL
			AvmAssert(info->optional_count == 0);
		}

		// now set up the required args (we can ignore argc)
		// for (int i=0, n=param_count; i <= n; i++)
		//     framep[i] = argv[i];
		int offset = 0;
		for (int i=0, n=info->param_count-info->optional_count; i <= n && !outOMem(); i++)
			copyParam(i, offset);

		if (info->flags & AbstractFunction::UNBOX_THIS)
		{
			localSet(0, atomToNativeRep(0, localGet(0)));
		}

		int firstLocal = 1+info->param_count;

		// capture remaining args
		if (info->flags & AbstractFunction::NEED_REST)
		{
			//framep[info->param_count+1] = createRest(env, argv, argc);
			// use csop so if rest value never used, we don't bother creating array
			LIns* rest = callIns(FUNCTIONID(createRestHelper), 3, 
				env_param, argc_param, apArg);

			localSet(firstLocal, rest);
			firstLocal++;
		}
		else if (info->flags & AbstractFunction::NEED_ARGUMENTS)
		{
			//framep[info->param_count+1] = createArguments(env, argv, argc);
			// use csop so if arguments never used, we don't create it
			LIns* arguments = callIns(FUNCTIONID(createArgumentsHelper), 3, 
				env_param, argc_param, apArg);

			localSet(firstLocal, arguments);
			firstLocal++;
		}

		if (firstLocal < state->verifier->local_count)
		{
			// set remaining locals to undefined
			for (int i=firstLocal, n=state->verifier->local_count; i < n && !outOMem(); i++)
			{
				if(!(state->value(i).traits == NULL)){ // expecting *
 					AvmAssertMsg(0,"(state->value(i).traits != NULL)");
 					return false; // fail verify
 				}
				localSet(i, undefConst);
			}
		}

		#ifdef DEBUGGER
		if (core->debugger())
		{
			for (int i=state->verifier->scopeBase; i<state->verifier->scopeBase+state->verifier->max_scope && !outOMem(); ++i)
			{
				localSet(i, undefConst);
			}

			callIns(FUNCTIONID(debugEnter), 8,
				env_param, argc_param, ap_param, 
				varTraits, InsConst(state->verifier->local_count), // for clearing traits pointers
				csn, 
				varPtrs,
				info->hasExceptions() ? _save_eip : InsConstPtr(0)
				);
		}
		#endif // DEBUGGER

		if (info->hasExceptions()) {
			// _ef.beginTry(core);
			callIns(FUNCTIONID(beginTry), 2, _ef, coreAddr);

			// Exception* setjmpResult = setjmp(_ef.jmpBuf);
			// ISSUE this needs to be a cdecl call
			LIns* jmpbuf = leaIns(offsetof(ExceptionFrame, jmpbuf), _ef);
			setjmpResult = callIns(FUNCTIONID(fsetjmp), 2,
				jmpbuf, InsConst(0));

			// if (setjmp() != 0) goto catch dispatcher, which we generate in the epilog.
			exBranch = branchIns(LIR_jf, binaryIns(LIR_eq, setjmpResult, InsConst(0)));
		}
        else {
            exBranch = 0;
        }

		// If interrupts are enabled, generate an interrupt check.
		// This ensures at least one interrupt check per method.
		if (interruptable && core->config.interrupts)
		{
			if (state->insideTryBlock)
				storeIns(InsConstPtr((void*)state->pc), 0, _save_eip);

			LIns* interrupted = loadIns(LIR_ld, 0, InsConstPtr(&core->interrupted));
			LIns* br = branchIns(LIR_jf, binaryIns(LIR_eq, interrupted, InsConst(0)));
			patchLater(br, interrupt_label);
		}
		verbose_only( if (vbWriter) { vbWriter->flush();} )
		return true;
	}

	void CodegenLIR::copyParam(int i, int& offset) {
		LIns* apArg = ap_param;
		switch (bt(info->paramTraits(i))) {
		case BUILTIN_number:
			localSet(i, loadIns(LIR_ldqc, offset, apArg));
			offset += sizeof(double);
			break;
		case BUILTIN_int:
		case BUILTIN_uint:
		case BUILTIN_boolean:
			// in the args these are widened to intptr_t or uintptr_t, so truncate here.
			localSet(i, p2i(loadIns(LIR_ldcp, offset, apArg)));
			offset += sizeof(Atom);
			break;
		default:
			localSet(i, loadIns(LIR_ldcp, offset, apArg));
			offset += sizeof(Atom);
			break;
		}
	}

	void CodegenLIR::emitCopy(FrameState* state, int src, int dest) {
		if (outOMem()) return;
		this->state = state;
		localSet(dest, localCopy(src));
	}

	void CodegenLIR::emitGetscope(FrameState* state, int scope_index, int dest)
	{
		if (outOMem()) return;
		this->state = state;
		Traits* t = info->declaringTraits->scope->getScopeTraitsAt(scope_index);
		LIns* declVTable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env_param);
		LIns* scope = loadIns(LIR_ldcp, offsetof(VTable, scope), declVTable);
		LIns* scopeobj = loadIns(LIR_ldcp, offsetof(ScopeChain, scopes) + scope_index*sizeof(Atom), scope);
		localSet(dest, atomToNativeRep(t, scopeobj));
	}

	void CodegenLIR::emitSwap(FrameState* state, int i, int j) {
		if (outOMem()) return;
		this->state = state;
		LIns* t = localCopy(i);
		localSet(i, localCopy(j));
		localSet(j, t);
	}

	void CodegenLIR::emitKill(FrameState* state, int i)
	{
		if (outOMem()) return;
		this->state = state;
		localSet(i, undefConst);
	}

	void CodegenLIR::emitSetContext(FrameState *state, AbstractFunction *f)
	{
		if (outOMem()) return;
		this->state = state;
		//
		// initialize the code context
		//
		if (isCodeContextChanged())
		{
 			if (!f || f->usesCallerContext())
			{
				// core->codeContext = env;
				storeIns(env_param, 0, InsConstPtr(&core->codeContextAtom));
			}

			if (!f || f->usesDefaultXmlNamespace())
			{
				emitSetDxns(state);

				// used to dump all functions that still say they required DXNS code
				#if 0//def _DEBUG
				if (f && (f->flags & AbstractFunction::NATIVE))
				{
					StringBuffer buffer(core);		
					buffer << "function is:" << f->name << "\r\n";

					AvmDebugMsg (false, buffer.c_str());
					//core->console << " f->
				}
				#endif
			}
		}
	}

	void CodegenLIR::emitSetDxns(FrameState* state)
	{
		if (outOMem()) return;
		this->state = state;

		LIns* dxnsAddr = dxns;

		if (!info->setsDxns()) {
			// dxnsAddr = &env->vtable->scope->defaultXmlNamespace
			LIns* env = env_param;
			LIns* declVTable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env);
			LIns* scope = loadIns(LIR_ldcp, offsetof(VTable, scope), declVTable);
			dxnsAddr = leaIns(offsetof(ScopeChain, defaultXmlNamespace), scope);
		}

		storeIns(dxnsAddr, 0, InsConstPtr(&core->dxnsAddr));
	}

	void CodegenLIR::merge(int i, const Value& current, Value& target) {
        copier->trackMerge(i, current.ins, target.ins);
	}


	void CodegenLIR::emitBlockStart(FrameState* state)
	{
		if (outOMem()) return;

		// our new extended BB now starts here, this means that any branch targets
		// should hit the next instruction our bb start instruction
		LIns* bb = Ins(LIR_label); // mark start of block
        verbose_only( if (frag->lirbuf->names) {
			char str[64];
			VMPI_sprintf(str,"B%d",(int)state->pc);
            frag->lirbuf->names->addName(bb, str);
        });

		// get a label for our block start and tie it to this location
		mirLabel(state->verifier->getFrameState(state->pc)->label, bb);

		// If this is a backwards branch, generate an interrupt check.
		// current verifier state, includes tack pointer.
		if (interruptable && core->config.interrupts && state->targetOfBackwardsBranch)
		{
			if (state->insideTryBlock)
				storeIns(InsConstPtr((void*)state->pc), 0, _save_eip);

			LIns* interrupted = loadIns(LIR_ld, 0, InsConstPtr(&core->interrupted));
			LIns* br = branchIns(LIR_jf, binaryIns(LIR_eq, interrupted, InsConst(0)));
			patchLater(br, interrupt_label);
		}

		// load undefined into any killed locals
		int stackBase = state->verifier->stackBase;
		for (int i=0, n=stackBase+state->stackDepth; i < n; i++)
		{
			int scopeTop = state->verifier->scopeBase+state->scopeDepth;
			if (i >= scopeTop && i < stackBase)
				continue; // not live
			Value &v = state->value(i);
			if (v.killed)
				localSet(i, undefConst);
		}
	}

	void CodegenLIR::emitBlockEnd(FrameState* state)
	{
		this->state = state;
		// our eBB now terminates.  For all branch instructions we are
		// able to detect this situation and have already generated the 
		// correct spill code prior to the jump, only for the case where 
		// the pc is a target of a jump do we not know enough during emit() 
		// to do this.  The Verifier tracks this information and merges
		// the states across the various blocks generating this op prior
		// to the merge.  After this emit, we should receive an emitBlockStart()
		// from the Verifier.
	}

	bool canAssign(Traits* lhs, Traits* rhs)
	{
		if (!Traits::isMachineCompatible(lhs,rhs))
		{
			// no machine type is compatible with any other 
			return false;
		}

		if (!lhs)
			return true;

		// type on right must be same class or subclass of type on left.
		Traits* t = rhs;
		while (t != lhs && t != NULL)
			t = t->base;
		return t != NULL;
	}

	void CodegenLIR::emitToString(AbcOpcode opcode, int i)
	{
		Traits *st = STRING_TYPE;
		Value& value = state->value(i);
		Traits *in = value.traits;
		if (in != st || !value.notNull || opcode != OP_convert_s)
		{
		    if (opcode == OP_convert_s && in && 
				(value.notNull || in->isNumeric() || in == BOOLEAN_TYPE))
			{
			    emitCoerce(state, i, st);
			}
			else
			{
			    emit(state, opcode, i, 0, st);
			}
			// FIXME side-effect!!!
			value.traits = st;
			value.notNull = true;
		}
	}

	void CodegenLIR::writePrologue(FrameState* state)
	{
	  prologue(state);
	}

	void CodegenLIR::writeEpilogue(FrameState* state)
	{
	  epilogue(state);
	}

	void CodegenLIR::write(FrameState* state, const byte* pc, AbcOpcode opcode)
	{
	  //printf("CodegenLIR::write %x\n", opcode);

		const byte* nextpc = pc;
		unsigned int imm30=0, imm30b=0;
		int imm8=0, imm24=0;
        AvmCore::readOperands(nextpc, imm30, imm24, imm30b, imm8);
		int sp = state->sp();

		switch (opcode) {
		case OP_nop:
		case OP_pop:
		case OP_label:
		    // do nothing
		    break;
		case OP_getlocal0:
		case OP_getlocal1:
		case OP_getlocal2:
		case OP_getlocal3:
		    imm30 = opcode-OP_getlocal0;
			// hack imm30 and fall through
		case OP_getlocal:
			emitCopy(state, imm30, sp+1);
			break;
		case OP_setlocal0:
		case OP_setlocal1:
		case OP_setlocal2:
		case OP_setlocal3:
			imm30 = opcode-OP_setlocal0;
			// hack imm30 and fall through
		case OP_setlocal:
			emitCopy(state, sp, imm30);
			break;
		case OP_pushtrue:
		    emitIntConst(state, sp+1, 1);
			break;
		case OP_pushfalse:
		    emitIntConst(state, sp+1, 0);
            break;
		case OP_pushnull:
			emitPtrConst(state, sp+1, 0);
			break;
		case OP_pushundefined:
			emitPtrConst(state, sp+1, (void*)undefinedAtom);
			break;
		case OP_pushshort:
			emitIntConst(state, sp+1, (signed short)imm30);
			break;
		case OP_pushbyte:
			emitIntConst(state, sp+1, (signed char)imm8);
			break;
	    case OP_pushstring:
			emitPtrConst(state, sp+1, pool->cpool_string[imm30]);
			break;
	    case OP_pushnamespace:
			emitPtrConst(state, sp+1, pool->cpool_ns[imm30]);
			break;
	    case OP_pushint:
			emitIntConst(state, sp+1, pool->cpool_int[imm30]);
			break;
	    case OP_pushuint:
			emitIntConst(state, sp+1, pool->cpool_uint[imm30]);
			break;
	    case OP_pushdouble:
			emitDoubleConst(state, sp+1, pool->cpool_double[imm30]);
			break;
		case OP_pushnan:
			emitDoubleConst(state, sp+1, (double*)(core->kNaN & ~7));
			break;
		case OP_lookupswitch: 
			emit(state, opcode, state->pc+imm24, imm30b /*count*/);
			break;
		case OP_throw:
		case OP_returnvalue:
		case OP_returnvoid:
			emit(state, opcode, sp);
			break;
		case OP_debugfile:
		{
#if defined(DEBUGGER) || defined(VTUNE)
			#ifdef VTUNE
			const bool do_emit = true;
			#else
			const bool do_emit = core->debugger() != NULL;
			#endif
		    Stringp str = pool->cpool_string[imm30];  // assume been checked already
			if(do_emit) emit(state, opcode, (uintptr)str);
#endif
			break;
		}
		case OP_dxns:
		{
		    Stringp str = pool->cpool_string[imm30];  // assume been checked already
			emit(state, opcode, (uintptr)str);
		    break;
		}
		case OP_dxnslate:
		    // codgen will call intern on the input atom.
		    emit(state, opcode, sp);
		    break;
		case OP_kill:
		    emitKill(state, imm30);
		    break;
		case OP_inclocal:
		case OP_declocal:
		    emit(state, opcode, imm30, opcode==OP_inclocal ? 1 : -1, NUMBER_TYPE);
		    break;
		case OP_inclocal_i:
		case OP_declocal_i:
		    emit(state, opcode, imm30, opcode==OP_inclocal_i ? 1 : -1, INT_TYPE);
			break;
		case OP_newfunction:
		    emitSetDxns(state);
			emit(state, opcode, imm30, sp+1, pool->methods[imm30]->declaringTraits);
			break;

		case OP_newclass:
		{
		    emitSetDxns(state);
		    AbstractFunction* cinit = pool->cinits[imm30];
			emit(state, opcode, (uintptr)(void*)cinit, sp, cinit->declaringTraits);
			break;
		}

		case OP_finddef: 
		{
  		    Multiname multiname;
		    pool->parseMultiname(multiname, imm30);
		    AbstractFunction* script = (AbstractFunction*)pool->getNamedScript(&multiname);
			if (script != (AbstractFunction*)BIND_NONE && script != (AbstractFunction*)BIND_AMBIGUOUS)
			{
			    // found a single matching traits
				emit(state, opcode, (uintptr)&multiname, sp+1, script->declaringTraits);
			}
			else
			{
			    // no traits, or ambiguous reference.  use Object, anticipating
			    // a runtime exception
			    emit(state, opcode, (uintptr)&multiname, sp+1, OBJECT_TYPE);
			}
			break;
		}

		case OP_findpropstrict: 
		case OP_findproperty: 
		{
  		    Multiname multiname;
		    pool->parseMultiname(multiname, imm30);
			emit(state, opcode, (uintptr)&multiname, 0, OBJECT_TYPE);
			break;
		}

		case OP_getdescendants:
		{
  		    Multiname multiname;
		    pool->parseMultiname(multiname, imm30);
			emit(state, opcode, (uintptr)&multiname, 0, NULL);
			break;
		}

		case OP_checkfilter:
		    emit(state, opcode, sp, 0, NULL);
			break;

		case OP_deleteproperty:
		{
  		    Multiname multiname;
		    pool->parseMultiname(multiname, imm30);
			emit(state, opcode, (uintptr)&multiname, 0, BOOLEAN_TYPE);
			break;
		}

		case OP_setproperty:
		case OP_initproperty:
		{
  		    Multiname multiname;
		    pool->parseMultiname(multiname, imm30);
		    emitSetContext(state, NULL);  // FIXME not be necessary in all code paths
			emit(state, opcode, (uintptr)&multiname);
			break;
		}

		case OP_astype:
		{
  		    Multiname name;
		    pool->parseMultiname(name, imm30);
		    Traits *t = pool->getTraits(name, state->verifier->getToplevel(this));
		    emit(state, OP_astype, (uintptr)t, sp, t && t->isMachineType() ? OBJECT_TYPE : t);
		    break;
		}
		case OP_astypelate:
		{
		    Value& classValue = state->peek(1); // rhs - class
			Traits* ct = classValue.traits;
			Traits* t = NULL;
			if (ct && (t=ct->itraits) != 0)
			    if (t->isMachineType())
				    t = OBJECT_TYPE;
			emit(state, opcode, 0, 0, t);
            break;
		}
		case OP_coerce:
		{
		    Traits *target = getType(imm30);
   			emitCoerce(state, sp, target);
		    break;
		}

		case OP_coerce_b:
		case OP_convert_b:
			emitCoerce(state, sp, BOOLEAN_TYPE);
		    break;

		case OP_coerce_o:
			emitCoerce(state, sp, OBJECT_TYPE);
		    break;

		case OP_coerce_a:
			emitCoerce(state, sp, NULL);
		    break;

		case OP_convert_i:
		case OP_coerce_i:
			emitCoerce(state, sp, INT_TYPE);
			break;

		case OP_convert_u:
		case OP_coerce_u:
			emitCoerce(state, sp, UINT_TYPE);
			break;

		case OP_convert_d:
		case OP_coerce_d:
			emitCoerce(state, sp, NUMBER_TYPE);
			break;

		case OP_coerce_s:
			emitCoerce(state, sp, STRING_TYPE);
			break;

		case OP_convert_s:
		case OP_esc_xelem: 
		case OP_esc_xattr:
		{
		    // the following was stolen from Verifier::emitToString
			Value& value = state->value(sp);
			Traits *in = value.traits;
		    if (opcode == OP_convert_s && in 
				&& (value.notNull || in->isNumeric() || in == BOOLEAN_TYPE))
			{
			    emitCoerce(state, sp, STRING_TYPE);
			}
			else
			{
			    emit(state, opcode, sp, 0, STRING_TYPE);
			}
			break;
		}

		case OP_istype:
		{
  		    Multiname name;
		    pool->parseMultiname(name, imm30);
		    Traits* itraits = pool->getTraits(name, state->verifier->getToplevel(this));
			emit(state, opcode, (uintptr)itraits, sp, BOOLEAN_TYPE);
		}

		case OP_istypelate: 
		    emit(state, opcode, 0, 0, BOOLEAN_TYPE);
			break;

		case OP_convert_o:
		    emitCheckNull(state, sp);
			break;

		case OP_callstatic:
		{
			AbstractFunction* m = pool->methods[imm30];
			const uint32_t argc = imm30b;
		    emitCheckNull(state, sp-argc);
			emitSetContext(state, m);
			emitCall(state, OP_callstatic, m->method_id, argc, m->returnTraits());
			break;
		}

		case OP_constructprop:
		{
			const uint32_t argc = imm30b;
  		    Multiname name;
		    pool->parseMultiname(name, imm30);
			emitSetContext(state, NULL);
			emit(state, opcode, (uintptr)&name, argc, NULL);
			break;
		}
		case OP_applytype:
		{
		    emitSetContext(state, NULL);
			// * is ok for the type, as Vector classes have no statics
			// when we implement type parameters fully, we should do something here.
			emit(state, opcode, imm30/*argc*/, 0, NULL);
		    break;
		}

		case OP_newobject: 
		    emit(state, opcode, imm30, 0, OBJECT_TYPE);
			break;

		case OP_newarray:
			emit(state, opcode, imm30, 0, ARRAY_TYPE);
			break;

		case OP_newactivation:
		    emit(state, opcode, 0, 0, info->activationTraits);
			break;

		case OP_newcatch:
		{
		    ExceptionHandler* handler = &info->exceptions->exceptions[imm30];
			emit(state, opcode, 0, 0, handler->scopeTraits);
			break;
		}

		case OP_popscope:
			#ifdef DEBUGGER
		    if (core->debugger()) emitKill(state, info->localCount/*scopeBase*/ + state->scopeDepth);
			#endif
			break;

		case OP_getslot:
		{
		    Value& obj = state->peek();
			int index = imm30-1;
			TraitsBindingsp td = obj.traits ? obj.traits->getTraitsBindings() : NULL;
			Traits* slotTraits = td->getSlotTraits(index);
		    emitCheckNull(state, sp);
            emitGetslot(state, index, sp, slotTraits);
			break;
        }

		case OP_setslot:
		{
		    Value& obj = state->peek(2);
			int index = imm30-1;
			TraitsBindingsp td = obj.traits ? obj.traits->getTraitsBindings() : NULL;
			Traits* slotTraits = td->getSlotTraits(index);
			emitCoerce(state, sp, slotTraits);
		    emitCheckNull(state, sp-1);
            emitSetslot(state, OP_setslot, index, sp-1);
			break;
		}

		case OP_dup:
		    emitCopy(state, sp, sp+1);
			break;

		case OP_swap:
		    emitSwap(state, sp, sp-1);
			break;

		case OP_lessthan:
		case OP_greaterthan:
		case OP_lessequals:
		case OP_greaterequals:
		{
		    Value& rhs = state->peek(1);
			Value& lhs = state->peek(2);
			Traits *lhst = lhs.traits;
			Traits *rhst = rhs.traits;
			if (rhst && rhst->isNumeric() && lhst && !lhst->isNumeric())
			{
				// convert lhs to Number
				//if ((!canAssign(NUMBER_TYPE, rhst) || !Traits::isMachineCompatible(NUMBER_TYPE, rhst)))
				    emitCoerce(state, sp-1, NUMBER_TYPE);
			}
			else if (lhst && lhst->isNumeric() && rhst && !rhst->isNumeric())
			{
				// promote rhs to Number
				//if ((!canAssign(NUMBER_TYPE, lhst) || !Traits::isMachineCompatible(NUMBER_TYPE, lhst)))
				    emitCoerce(state, sp, NUMBER_TYPE);
			}
			emit(state, opcode, 0, 0, BOOLEAN_TYPE);
		    break;
		}

		case OP_equals:
		case OP_strictequals:
		case OP_instanceof:
		case OP_in:
		    emit(state, opcode, 0, 0, BOOLEAN_TYPE);
		    break;

		case OP_not:
		{
		    emitCoerce(state, sp, BOOLEAN_TYPE);
		    emit(state, opcode, sp);
		    break;
		}

		case OP_add:
		{
		    Value& rhs = state->peek(1);
			Value& lhs = state->peek(2);
			Traits* lhst = lhs.traits;
			Traits* rhst = rhs.traits;
			// Note that for correctness the inference of the result type must
			// remain even in the non-JIT case
			if (lhst == STRING_TYPE && lhs.notNull || rhst == STRING_TYPE && rhs.notNull)
			{
			    emitToString(OP_convert_s, sp-1);
				emitToString(OP_convert_s, sp);
				emit(state, OP_concat, 0, 0, STRING_TYPE);
			}
			else if (lhst && lhst->isNumeric() && rhst && rhst->isNumeric())
			{
			    emitCoerce(state, sp-1, NUMBER_TYPE);
			    emitCoerce(state, sp, NUMBER_TYPE);
				emit(state, OP_add_d, 0, 0, NUMBER_TYPE);
			}
			else
		    {
			    emit(state, OP_add, 0, 0, OBJECT_TYPE);
				// dont know if it will return number or string, but neither will be null.
			}
			break;
		}

		case OP_modulo:
		case OP_subtract:
		case OP_divide:
		case OP_multiply:
		{
			emitCoerce(state, sp-1, NUMBER_TYPE); // convert LHS to number
			emitCoerce(state, sp, NUMBER_TYPE); // convert RHS to number
			emit(state, opcode, 0, 0, NUMBER_TYPE);
			break;
		}

		case OP_increment:
		case OP_decrement:
		    emitCoerce(state, sp, NUMBER_TYPE);
			emit(state, opcode, sp, opcode == OP_increment ? 1 : -1, NUMBER_TYPE);
			break;

		case OP_increment_i:
		case OP_decrement_i:
		    emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, sp, opcode == OP_increment_i ? 1 : -1, INT_TYPE);
			break;

		case OP_add_i:
		case OP_subtract_i:
		case OP_multiply_i:
		    emitCoerce(state, sp-1, INT_TYPE);
			emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, 0, 0, INT_TYPE);
			break;

		case OP_negate:
			emitCoerce(state, sp, NUMBER_TYPE);
			emit(state, opcode, sp, 0, NUMBER_TYPE);
			break;

		case OP_negate_i:
			emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, sp, 0, INT_TYPE);
			break;

		case OP_bitand:
		case OP_bitor:
		case OP_bitxor:
		    emitCoerce(state, sp-1, INT_TYPE);
			emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, 0, 0, INT_TYPE);
			break;

		case OP_lshift:
		case OP_rshift:
		    emitCoerce(state, sp-1, INT_TYPE); // lhs
			emitCoerce(state, sp, UINT_TYPE); // rhs
			emit(state, opcode, 0, 0, INT_TYPE);
			break;

		case OP_urshift:
		    emitCoerce(state, sp-1, UINT_TYPE); // lhs
			emitCoerce(state, sp, UINT_TYPE); // rhs
			emit(state, opcode, 0, 0, INT_TYPE);
			break;

		case OP_bitnot:
			emitCoerce(state, sp, INT_TYPE); // lhs
			emit(state, opcode, sp, 0, INT_TYPE);
			break;

		case OP_typeof:
			emit(state, opcode, sp, 0, STRING_TYPE);
			break;

		case OP_debugline:
		{
            #if defined(DEBUGGER) || defined(VTUNE)
			#ifdef VTUNE
			const bool do_emit = true;
			#else
			const bool do_emit = core->debugger() != NULL;
			#endif
		    // we actually do generate code for these, in debugger mode
		    if (do_emit) emit(state, opcode, imm30);
            #endif
			break;
		}
		case OP_nextvalue:
		case OP_nextname:
		    emit(state, opcode, 0, 0, NULL);
			break;

		case OP_hasnext:
		    emit(state, opcode, 0, 0, INT_TYPE);
			break;

		case OP_hasnext2:
		    emit(state, opcode, imm30, imm30b, BOOLEAN_TYPE);
			break;

#ifdef AVMPLUS_MOPS
		// sign extends
		case OP_sxi1:
		case OP_sxi8:
		case OP_sxi16:
		    emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, sp, 0, INT_TYPE);
			break;

		// loads
		case OP_li8:
		case OP_li16:
		case OP_li32:
		case OP_lf32:
		case OP_lf64:
		{
			Traits* result = (opcode == OP_lf32 || opcode == OP_lf64) ? NUMBER_TYPE : INT_TYPE;
			emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, sp, 0, result);
			break;
		}

		// stores
		case OP_si8:
		case OP_si16:
		case OP_si32:
		case OP_sf32:
		case OP_sf64:
		{
			emitCoerce(state, sp-1, (opcode == OP_sf32 || opcode == OP_sf64) ? NUMBER_TYPE : INT_TYPE);
			emitCoerce(state, sp, INT_TYPE);
			emit(state, opcode, 0, 0, VOID_TYPE);
			break;
		}

#endif // AVMPLUS_MOPS

		case OP_getouterscope:
			//assert (info->declaringTraits->scope->size > 0);
		    emitGetscope(state, imm30, sp+1);
			break;

		case OP_getglobalscope:
		    emitGetGlobalScope();
			break;

		default:
			// FIXME need error handler here
		    break;
		}
	}

	void CodegenLIR::emitGetGlobalScope()
	{
		ScopeTypeChain* scope = info->declaringTraits->scope;
		int captured_depth = scope->size;
		if (captured_depth > 0)
		{
			// enclosing scope
			emitGetscope(state, 0, state->sp()+1);
		}
		else
		{
			// local scope
			if (state->scopeDepth > 0)
			{
				emitCopy(state, state->verifier->scopeBase, state->sp()+1);
				// this will copy type and all attributes too
			}
			else
			{
				#ifdef _DEBUG
				if (pool->isBuiltin)
					core->console << "getglobalscope >= depth (0) "<< state->scopeDepth << "\n";
				#endif
			}
		}
	}

	void CodegenLIR::writeOp1(FrameState* state, const byte *pc, AbcOpcode opcode, uint32_t opd1, Traits *type)
	{
		(void)pc;
		switch (opcode) {
		case OP_iflt:
		case OP_ifle:
		case OP_ifnlt:
		case OP_ifnle:
		case OP_ifgt:
		case OP_ifge:
		case OP_ifngt:
		case OP_ifnge:
		case OP_ifeq:
		case OP_ifstricteq:
		case OP_ifne:
		case OP_ifstrictne:
		{
			int32_t offset = (int32_t) opd1;
			int lhs = state->sp()-1;
			emitIf(state, opcode, state->pc+4/*size*/+offset, lhs, lhs+1);
			break;
		}
		case OP_iftrue:
		case OP_iffalse:
		{
			int32_t offset = (int32_t) opd1;
			int sp = state->sp();
			emitCoerce(state, sp, BOOLEAN_TYPE);
			emitIf(state, opcode, state->pc+4/*size*/+offset, sp, 0);
			break;
		}
		case OP_jump:
		{
			int32_t offset = (int32_t) opd1;
		    emit(state, opcode, state->pc+4/*size*/+offset);
			break;
        }
		case OP_getslot:
		    emitGetslot(state, opd1, state->sp(), type);
			break;
		case OP_getglobalslot:
		    emitGetGlobalScope();
			emitCheckNull(state, state->sp());
		    emitGetslot(state, opd1, state->sp(), type);
			break;
		case OP_setglobalslot:
		{
		    // FIXME untested
			int sp = state->sp();    
			emitCoerce(state, sp, type);
			emit(state, opcode, opd1, sp, type);
			break;
		}
		case OP_getproperty:
		{
		    Multiname name;
			pool->parseMultiname(name, opd1);
            emit(state, OP_getproperty, (uintptr)&name, 0, type);
			break;
		}
		case OP_call:
		    emitSetContext(state, NULL);
			emit(state, opcode, opd1 /*argc*/, 0, NULL);
			break;

		case OP_construct:
		{
			const uint32_t argc = opd1;
  		    Traits* ctraits = state->peek(argc+1).traits;
			// don't need null check, AvmCore::construct() uses toFunction() for null check.
			Traits* itraits = ctraits ? ctraits->itraits : NULL;
			emitSetContext(state, NULL);
			emit(state, opcode, argc, 0, itraits);
			break;
		}
		case OP_getouterscope:
		    emitGetscope(state, opd1, state->sp()+1);
			break;
		case OP_getscopeobject:
		    emitCopy(state, opd1+state->verifier->scopeBase, state->sp()+1);
			break;
		default:
			// FIXME need error handler here
		    break;
		}
	}

	void CodegenLIR::writeOp2(FrameState* state, const byte *pc, AbcOpcode opcode, uint32_t opd1, uint32_t opd2, Traits *type)
	{
		(void)pc;
		switch (opcode) {
		case OP_setslot:
		    emitSetslot(state, OP_setslot, opd1, opd2);
			break;

		case OP_abs_jump:
		{
            #ifdef AVMPLUS_64BIT
            const byte* nextpc = pc;
            unsigned int imm30=0, imm30b=0;
            int imm8=0, imm24=0;
            AvmCore::readOperands(nextpc, imm30, imm24, imm30b, imm8);
            const byte* new_pc = (const byte *) (uintptr(opd1) | (((uintptr) opd2) << 32));
            const byte* new_code_end = new_pc + AvmCore::readU30 (nextpc);
            #else
			const byte* new_pc = (const byte*) opd1;
			const byte* new_code_end = new_pc + opd2;
			#endif
            this->abcStart = new_pc;
            this->abcEnd = new_code_end;
		    break;
		}

		case OP_callmethod:
		    emitCall(state, opcode, opd1, opd2, type);
		    break;

		case OP_callproperty: 
		case OP_callproplex: 
		case OP_callpropvoid:
		{
		    Multiname name;
			pool->parseMultiname(name, opd1);
		    emitSetContext(state, NULL);
			emit(state, opcode, (uintptr)&name, opd2, NULL);
			break;
		}

		case OP_callstatic:
		{
		    emitCheckNull(state, state->sp()-opd2);
			emitSetContext(state, pool->methods[opd1]);
			emitCall(state, OP_callstatic, opd1, opd2, type);
			break;
		}

		default:
			// FIXME need error handler here
		    break;
		}
	}

	Traits* CodegenLIR::getType(uint32_t index)
	{
	    Toplevel* toplevel = state->verifier->getToplevel(this);
		Multiname name;
		pool->parseMultiname(name, index);
		Traits *t = pool->getTraits(name, toplevel);
		if( name.isParameterizedType() )
		{
			Traits* param_traits = name.getTypeParameter() ? getType(name.getTypeParameter()) : NULL ;
			t = pool->resolveParameterizedType(toplevel, t, param_traits);
		}
		return t;
	}

	void CodegenLIR::emitIntConst(FrameState* state, int index, int32_t c)
	{
		if (outOMem()) return;
		this->state = state;
		localSet(index, lirout->insImm(c));
	}

	void CodegenLIR::emitPtrConst(FrameState* state, int index, void* c)
	{
		if (outOMem()) return;
		this->state = state;
		localSet(index, lirout->insImmPtr(c));
	}

	void CodegenLIR::emitDoubleConst(FrameState* state, int index, double* pd)
	{
		if (outOMem()) return;
		this->state = state;
        uint64_t *pquad = (uint64_t*) pd;
		localSet(index, lirout->insImmq(*pquad));
	}

	void CodegenLIR::emitCoerce(FrameState* state, int loc, Traits* result)
	{
		if (outOMem()) return;
		this->state = state;
		emitPrep();

		Value& value = state->value(loc);
		Traits* in = value.traits;

	    if (canAssign(result, in) && Traits::isMachineCompatible(result, in)) {
		    state->setType(loc, result, value.notNull);
		    return;
		}

		if (result == NULL)
		{
			// coerce to * is simple, we just save the atom rep.
			localSet(loc, loadAtomRep(loc));
		}
		else if (result == OBJECT_TYPE)
		{
			if (in == NULL || in == VOID_TYPE)
			{
				// value already boxed but we need to coerce undefined->null
                if (!value.notNull) {
                    // v == undefinedAtom ? nullObjectAtom : v;
                    LIns *v = localGetp(loc);
                    v = lirout->ins_choose(binaryIns(LIR_peq, v, undefConst),
                        InsConstAtom(nullObjectAtom), v);
                    localSet(loc, v);
                }
			}
			else
			{
				// value cannot be undefined so just box it
				localSet(loc, loadAtomRep(loc));
			}
		}
		else if (!result->isMachineType() && in == NULL_TYPE)
		{
			// do nothing, it's fine to coerce null to a pointer type
		}
		else if (result == NUMBER_TYPE)
		{
			if (in && in->isNumeric() || in == BOOLEAN_TYPE)
			{
				localSet(loc, promoteNumberIns(in, loc));
			}
			else
			{
				// * -> Number
				localSet(loc, callIns(FUNCTIONID(number), 2,
					coreAddr, loadAtomRep(loc)));
			}
		}
		else if (result == INT_TYPE)
		{
			if (in == UINT_TYPE || in == BOOLEAN_TYPE)
			{
				//do nothing
			}
			else if (in == NUMBER_TYPE)
			{
				// narrowing conversion number->int
				LIns* ins = localGetq(loc);
				localSet(loc, callIns(FUNCTIONID(integer_d), 1, ins));
			}
			else
			{
				// * -> int
				localSet(loc, callIns(FUNCTIONID(integer), 2,
					coreAddr, loadAtomRep(loc)));
			}
		}
		else if (result == UINT_TYPE)
		{
			if (in == INT_TYPE || in == BOOLEAN_TYPE)
			{
				//do nothing
			}
			else if (in == NUMBER_TYPE)
			{
				LIns* ins = localGetq(loc);
				localSet(loc, callIns(FUNCTIONID(integer_d), 1, ins));
			}
			else
			{
				// * -> uint
				localSet(loc, callIns(FUNCTIONID(toUInt32), 2,
					coreAddr, loadAtomRep(loc)));
			}
		}
		else if (result == BOOLEAN_TYPE)
		{
			if (in == NUMBER_TYPE)
			{
                localSet(loc, callIns(FUNCTIONID(doubleToBool), 1, localGetq(loc)));
			}
			else if (in == INT_TYPE || in == UINT_TYPE)
			{
                // int to bool: b = (i==0) == 0
				localSet(loc, binaryIns(LIR_eq, 
                    binaryIns(LIR_eq, localGet(loc), InsConst(0)),
                    InsConst(0)));
			}
            else if (in && !in->notDerivedObjectOrXML())
            {
                // ptr to bool: b = (p==0) == 0
                localSet(loc, binaryIns(LIR_eq,
                                        binaryIns(LIR_peq, localGetp(loc), InsConstPtr(0)),
                                        InsConst(0)));
            }
			else
			{
				// * -> Boolean
				localSet(loc, callIns(FUNCTIONID(boolean), 2,
					coreAddr, loadAtomRep(loc)));
			}
		}
		else if (result == STRING_TYPE)
		{
			if (in == INT_TYPE)
			{
				localSet(loc, callIns(FUNCTIONID(intToString), 2,
					coreAddr, localGet(loc)));
			}
			else if (in == UINT_TYPE)
			{
				localSet(loc, callIns(FUNCTIONID(uintToString), 2,
					coreAddr, localGet(loc)));
			}
			else if (in == NUMBER_TYPE)
			{
				localSet(loc, callIns(FUNCTIONID(doubleToString), 2,
					coreAddr, localGetq(loc)));
			}
			else if (in == BOOLEAN_TYPE)
			{
				// load "true" or "false"
				LIns *index = binaryIns(LIR_pilsh, i2p(localGet(loc)), InsConst(PTR_SCALE));
				LIns *arr = InsConstPtr(&core->booleanStrings);
				localSet(loc, loadIns(LIR_ldcp, 0, binaryIns(LIR_addp, arr, index)));
			}
			else if (value.notNull)
			{
				// not eligible for CSE, and we know it's not null/undefined
				localSet(loc, callIns(FUNCTIONID(string), 2,
					coreAddr, loadAtomRep(loc)));
			}
			else
			{
				localSet(loc, callIns(FUNCTIONID(coerce_s), 2,
					coreAddr, loadAtomRep(loc)));
			}
		}
		else if (in && !in->isMachineType() && !result->isMachineType()
			   && in != STRING_TYPE && in != NAMESPACE_TYPE)
		{
			LIns* toplevel = loadToplevel();
			// coerceobj is void, but we mustn't optimize it out; verifier only calls it when required
			callIns(FUNCTIONID(coerceobj), 3,
				toplevel, localGetp(loc), InsConstPtr(result));
			// the input pointer has now been checked but it's still the same value.
			// verifier remembers this fact by updating the verify time type.
		}
		else if (!result->isMachineType() && result != NAMESPACE_TYPE)
		{
			// result is a ScriptObject based type.
			localSet(loc, callIns(FUNCTIONID(coerceAtom2SO), 3,
				env_param, loadAtomRep(loc), InsConstPtr(result)));
		}
		else
		{
			LIns* value = loadAtomRep(loc);
			LIns* toplevel = loadToplevel();
			// sp[0] = toplevel->coerce(sp[0], traits)
			LIns* out = callIns(FUNCTIONID(coerce), 3,
				toplevel,
				value,
				InsConstPtr(result));

			// store the result
			localSet(loc, atomToNativeRep(result, out));
		}

		state->setType(loc, result, value.notNull);
	}

	void CodegenLIR::emitCheckNull(FrameState* state, int index)
	{
	    if (state->value(index).notNull) return;
		if (outOMem()) return;
		this->state = state;
		emitPrep();

		// The result is either unchanged or an exception is thrown, so 
		// we don't save the result.  This is the null pointer check.
		Traits* t = state->value(index).traits;
		switch (bt(t)) {
			case BUILTIN_any:
			case BUILTIN_object:
			case BUILTIN_void: {
				// Atom
				// checking atom for null or undefined (AvmCore::isNullOrUndefined())
				LIns *value = localGetp(index);
				callIns(FUNCTIONID(nullcheck), 2, env_param, value);
				break;
			}
			case BUILTIN_int:
			case BUILTIN_uint:
			case BUILTIN_boolean:
			case BUILTIN_number:
				// never null
				break;
			default: {
				// checking pointer for null
				LIns *value = localGetp(index);
				LIns* br = branchIns(LIR_jt, binaryIns(LIR_peq, value, InsConstPtr(0))); // will be patched
				patchLater(br, npe_label);
			}
		}
		// else: number, int, uint, and boolean, are never null
	}

	void CodegenLIR::emitPrep()
	{
		// update bytecode ip if necessary
		if (state->insideTryBlock && lastPcSave != state->pc)
		{
			storeIns(InsConstPtr((void*)state->pc), 0, _save_eip);
			lastPcSave = state->pc;
		}
	}

	void CodegenLIR::emitCall(FrameState *state, AbcOpcode opcode, intptr_t method_id, int argc, Traits* result)
	{
		if (outOMem()) return;
		this->state = state;
		emitPrep();

		int sp = state->sp();

		int dest = sp-argc;
		int objDisp = dest;

		// make sure null check happened.
		AvmAssert(state->value(objDisp).notNull);

		LIns *method = NULL;
		LIns *iid = NULL;
		switch (opcode)
		{
		case OP_constructsuper:
		{
			// env->vtable->base->init->enter32v(argc, ...);
			LIns* envArg = env_param;
			LIns* vtable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), envArg);
			LIns* base = loadIns(LIR_ldcp, offsetof(VTable,base), vtable);
			method = loadIns(LIR_ldcp, offsetof(VTable,init), base);
			break;
		}
		case OP_callmethod:
		{
			// stack in:  obj arg1..N
			// stack out: result
			// sp[-argc] = callmethod(disp_id, argc, ...);
			// method_id is disp_id of virtual method
			LIns* vtable = loadVTable(objDisp);
			method = loadIns(LIR_ldcp, offsetof(VTable, methods)+sizeof(uintptr)*method_id, vtable);
			break;
		}
		case OP_callsuperid:
		{
			// stack in: obj arg1..N
			// stack out: result
			// method_id is disp_id of super method
			LIns* declvtable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env_param);
			LIns* basevtable = loadIns(LIR_ldcp, offsetof(VTable, base), declvtable);
			method = loadIns(LIR_ldcp, offsetof(VTable, methods)+sizeof(uintptr)*method_id, basevtable);
			break;
		}
		case OP_callstatic:
		{
			// stack in: obj arg1..N
			// stack out: result

			LIns* vtable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env_param);
			LIns* abcenv = loadIns(LIR_ldcp, offsetof(VTable, abcEnv), vtable);
			method = loadIns(LIR_ldcp, offsetof(AbcEnv,m_methods)+sizeof(uintptr)*method_id, abcenv);
			break;
		}
		case OP_callinterface:
		{
			// method_id is pointer to interface method name (multiname)
			int index = int(method_id % Traits::IMT_SIZE);
			LIns* vtable = loadVTable(objDisp);
			method = loadIns(LIR_ldcp, offsetof(VTable, imt)+sizeof(uintptr)*index, vtable);
			iid = InsConstPtr((void*)method_id);
			break;
		}
		case OP_construct:
		{
			// stack in: ctor arg1..N
			// stack out: newinstance
			LIns* vtable = loadVTable(objDisp);
			LIns* ivtable = loadIns(LIR_ldcp, offsetof(VTable, ivtable), vtable);
			method = loadIns(LIR_ldcp, offsetof(VTable, init), ivtable);
			LIns* inst = callIns(FUNCTIONID(newInstance),1, localGetp(objDisp));
			localSet(dest, inst);
			break;
		}
		default:
			AvmAssert(false);
		}

		// store args for the call
		int index = objDisp;
		LIns* ap = InsAlloc(sizeof(Atom)); // we will update this size, below
		int disp = 0;
        int pad = 0;

        // LIR_alloc of any size >= 8 is always 8-aligned.
        // if the first double arg would be unaligned, add padding to align it.
	#if !defined NANOJIT_64BIT
        for (int i=0; i <= argc; i++) {
            if (state->value(index+i).traits == NUMBER_TYPE) {
                if ((disp&7) != 0) {
                    // this double would be unaligned, so add some padding
                    pad = 8-(disp&7); // should be 4
                }
                break;
            }
            else {
                disp += sizeof(Atom);
            }
        }
	#endif
        
        disp = pad;
		for (int i=0; i <= argc && !outOMem(); i++)
		{
			// load and widen the argument
			LIns *v;
			switch (bt(state->value(index).traits)) {
			case BUILTIN_number:
				v = localGetq(index);
				break;
			case BUILTIN_int:
				v = i2p(localGet(index));
				break;
			case BUILTIN_uint:
			case BUILTIN_boolean:
				v = u2p(localGet(index));
				break;
			default:
				v = localGetp(index);
				break;
			}
			index++;
			storeIns(v, disp, ap);
            disp += v->isQuad() ? sizeof(double) : sizeof(Atom);
		}

        // patch the size to what we actually need
		ap->setSize(disp);

		LIns* target = loadIns(LIR_ldp, offsetof(MethodEnv, impl32), method);
		LIns* apAddr = leaIns(pad, ap);

        LIns *out;
		BuiltinType rbt = bt(result);
        if (!iid) {
			const CallInfo *fid;
			switch (rbt) {
			case BUILTIN_number:
				fid = FUNCTIONID(fcalli);
				break;
            case BUILTIN_int: case BUILTIN_uint: case BUILTIN_boolean:
				fid = FUNCTIONID(icalli);
				break;
			default:
				fid = FUNCTIONID(acalli);
				break;
			}
		    out = callIns(fid, 4, target, method, InsConst(argc), apAddr);
        } else {
			const CallInfo *fid;
			switch (rbt) {
			case BUILTIN_number:
				fid = FUNCTIONID(fcallimt);
				break;
            case BUILTIN_int: case BUILTIN_uint: case BUILTIN_boolean:
				fid = FUNCTIONID(icallimt);
				break;
			default:
				fid = FUNCTIONID(acallimt);
				break;
			}
		    out = callIns(fid, 5, target, iid, method, InsConst(argc), apAddr);
        }

		if (opcode != OP_constructsuper && opcode != OP_construct)
		{
			localSet(dest, out); 
		}
	}

    void CodegenLIR::emitGetslot(FrameState *state, int slot, int ptr_index, Traits *result)
    {
		if (outOMem()) return;
        this->state = state;
        emitPrep();
        PERFM_NVPROF("emitGetslot",1);
		
		Traits *t = state->value(ptr_index).traits;
		LIns *ptr = localGetp(ptr_index);
		AvmAssert(state->value(ptr_index).notNull);
		AvmAssert(isPointer((int)ptr_index)); // obj

		AvmAssert(t->isResolved());
		const TraitsBindingsp tb = t->getTraitsBindings();
		int offset = tb->getSlotOffset(slot);
		
		if (t->pool->isBuiltin && !t->final)
		{
			// t's slots aren't locked in, so we have to adjust for the actual runtime
			// traits->sizeofInstance.
			LIns* vtable = loadIns(LIR_ldcp, offsetof(ScriptObject,vtable), ptr);
			LIns* traits = loadIns(LIR_ldcp, offsetof(VTable,traits), vtable);
			offset -= (int)(t->m_sizeofInstance);
			LIns* sizeofInstance = loadIns(LIR_ldc, offsetof(Traits, m_sizeofInstance), traits);
			ptr = binaryIns(LIR_addp, u2p(sizeofInstance), ptr);
		}

		// get
		LOpcode op;
		switch (bt(result)) {
		case BUILTIN_number:	op = LIR_ldq;	break;
		case BUILTIN_int:
		case BUILTIN_uint:
		case BUILTIN_boolean:	op = LIR_ld;	break;
		default:				op = LIR_ldp;	break;
		}
		localSet(ptr_index, loadIns(op, offset, ptr));
    }

    void CodegenLIR::emitSetslot(FrameState *state, AbcOpcode opcode, int slot, int ptr_index)
    {
		if (outOMem()) return;
        this->state = state;
        emitPrep();
        int sp = state->sp();

        PERFM_NVPROF("emitSetslot",1);
		Traits* t;
		LIns* ptr;
		
		if (opcode == OP_setslot)
		{
			t = state->value(ptr_index).traits;
			ptr = localGetp(ptr_index);
			AvmAssert(state->value(ptr_index).notNull);
			AvmAssert(isPointer((int)ptr_index)); // obj
		}
		else
		{
            // setglobalslot
			ScopeTypeChain* scopeTypes = info->declaringTraits->scope;
			if (scopeTypes->size == 0)
			{
				// no captured scopes, so global is local scope 0
				ptr_index = state->verifier->scopeBase;
				t = state->value(ptr_index).traits;
				ptr = localGetp(ptr_index);
				AvmAssert(state->value(ptr_index).notNull);
				AvmAssert(isPointer((int)ptr_index)); // obj
			}
			else
			{
                // global is outer scope 0
				t = scopeTypes->getScopeTraitsAt(0);
				LIns* declVTable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env_param);
				LIns* scope = loadIns(LIR_ldcp, offsetof(VTable, scope), declVTable);
				LIns* scopeobj = loadIns(LIR_ldp, offsetof(ScopeChain, scopes) + 0*sizeof(Atom), scope);
				ptr = atomToNativeRep(t, scopeobj);
			}				
		}

		AvmAssert(t->isResolved());
		const TraitsBindingsp tb = t->getTraitsBindings();
		int offset = tb->getSlotOffset(slot);
		
		LIns *unoffsetPtr = ptr;
		if (t->pool->isBuiltin && !t->final)
		{
			// t's slots aren't locked in, so we have to adjust for the actual runtime
			// traits->sizeofInstance.
			LIns* vtable = loadIns(LIR_ldcp, offsetof(ScriptObject,vtable), ptr);
			LIns* traits = loadIns(LIR_ldcp, offsetof(VTable,traits), vtable);
			offset -= (int)(t->m_sizeofInstance);
			LIns* sizeofInstance = loadIns(LIR_ldc, offsetof(Traits, m_sizeofInstance), traits);
			ptr = binaryIns(LIR_addp, u2p(sizeofInstance), ptr);
		}

		// set
        // use localCopy() to sniff the type and use ldq if it's Number
		LIns* value = localCopy(sp);					

		#if !defined WRITE_BARRIERS || !defined MMGC_DRC
			#error "untested in years"
		#endif

		// the following code assumes WRITE_BARRIERS and MMGC_DRC are enabled

		// if storing to a pointer-typed slot, inline a WB
		Traits* slotType = tb->getSlotTraits(slot);

		if (core->GetGC()->incremental &&
			(!slotType || !slotType->isMachineType() || slotType == OBJECT_TYPE))
		{
			const CallInfo *wbAddr = FUNCTIONID(writeBarrierRC);
			if(slotType == NULL || slotType == OBJECT_TYPE) {
				// use fast atom wb
				// TODO: inline pointer check
				wbAddr = FUNCTIONID(atomWriteBarrier);
			}
			callIns(wbAddr, 4, 
					InsConstPtr(core->GetGC()), 
					unoffsetPtr, 
					leaIns(offset, ptr),
					value);
		}					
		else {
			storeIns(value, offset, ptr);
		}
    }

    typedef const CallInfo *CallInfop;

	void CodegenLIR::emit(FrameState* state, AbcOpcode opcode, uintptr op1, uintptr op2, Traits* result)
	{
		if (outOMem()) return;
		this->state = state;
		emitPrep();

		int sp = state->sp();

		switch (opcode)
		{
#ifdef AVMPLUS_MOPS
			// sign extends
			case OP_sxi1:
			case OP_sxi8:
			case OP_sxi16:
			{
				// straightforward shift based sign extension
				static const uint8_t kShiftAmt[3] = { 31, 24, 16 };
				int32_t index = (int32_t) op1;
				LIns* val = localGet(index);
				LIns* sh = InsConst(kShiftAmt[opcode - OP_sxi1]);
				LIns* shl = binaryIns(LIR_lsh, val, sh);
				LIns* res = binaryIns(LIR_rsh, shl, sh);
				localSet(index, res);
				break;
			}
			
			// loads
			case OP_li8:
			case OP_li16:
			case OP_li32:
			case OP_lf32:
			case OP_lf64:
			{
				static const CallInfop kFuncID[5] = { 
					FUNCTIONID(li8),  
					FUNCTIONID(li16),  
					FUNCTIONID(li32),  
					FUNCTIONID(lf32),  
					FUNCTIONID(lf64)
				};

				int32_t index = (int32_t) op1;
				LIns* addr = localGet(index);
				LIns* i2 = callIns(kFuncID[opcode-OP_li8], 2, env_param, addr);
				localSet(index, i2);
				break;
			}
			
			// stores
			case OP_si8:
			case OP_si16:
			case OP_si32:
			case OP_sf32:
			case OP_sf64:
			{
				static const CallInfop kFuncID[5] = { 
					FUNCTIONID(si8),  
					FUNCTIONID(si16),  
					FUNCTIONID(si32),  
					FUNCTIONID(sf32),  
					FUNCTIONID(sf64)
				};

				LIns* svalue = (opcode == OP_sf32 || opcode == OP_sf64) ? localGetq(sp-1) : localGet(sp-1);
				LIns* addr = localGet(sp);
				callIns(kFuncID[opcode-OP_si8], 3, env_param, svalue, addr);
				break;
			}
#endif

			case OP_jump:
			{
                PERFM_NVPROF("emit(jump",1);
				// spill everything first
				intptr_t targetpc = op1;

#ifdef DEBUGGER
				Sampler* s = core->get_sampler();
				if (s && s->sampling() && targetpc < state->pc)
				{
					emitSampleCheck();
				}
#endif

				// relative branch
				branchIns(LIR_j, 0, targetpc); // will be patched
				break;
			}

			case OP_lookupswitch:
			{
                PERFM_NVPROF("emit(switch",1);
				//int index = integer(*(sp--));
				//pc += readS24(index < readU16(pc+4) ? 
				//	(pc+6+3*index) :	// matched case
				//	(pc+1));			// default
				int count = int(1 + op2);
				intptr_t targetpc = op1;

				AvmAssert(state->value(sp).traits == INT_TYPE);

				LIns* input = localGet(sp);
				LIns* cmp = binaryIns(LIR_ult, input, InsConst(count));
				branchIns(LIR_jf, cmp, targetpc); // will be patched

                // fixme - this is just a bunch of if's

				const byte* pc = 4 + abcStart + state->pc;
                AvmCore::readU30(pc);

				for (int i=0; i < count && !outOMem(); i++)
				{
					intptr_t target = state->pc + AvmCore::readS24(pc+3*i);
                    branchIns(LIR_jt, binaryIns(LIR_eq, input, InsConst(i)), target);
				}
				break;
			}

			case OP_returnvalue:
				emitCoerce(state, sp, info->returnTraits());
				// fall through
			case OP_returnvoid:
			{
                PERFM_NVPROF("emit(return",1);
				// ISSUE if a method has multiple returns this causes some bloat

				// restore AvmCore::dxnsAddr if we set it to a stack addr in our frame
				if(info->setsDxns()) {
					storeIns(dxnsAddrSave, 0, InsConstPtr(&core->dxnsAddr));
				}

				#ifdef DEBUGGER
				if (core->debugger())
				{
					callIns(FUNCTIONID(debugExit), 2,
						env_param, csn);
					// now we toast the cse and restore contents in order to 
					// ensure that any variable modifications made by the debugger
					// will be pulled in.
					//firstCse = ip;
				}
				#endif // DEBUGGER

				if (info->exceptions)
				{
					// _ef.endTry();
					callIns(FUNCTIONID(endTry), 1, _ef);
				}

				Traits* t = info->returnTraits();
				LIns* retvalue;
				if (opcode == OP_returnvalue)
				{
					// already coerced to required native type
                    // use localCopy to sniff type and use appropriate load instruction
                    int32_t index = (int32_t) op1;
					retvalue = localCopy(index);
				}
				else
				{
					retvalue = undefConst;
					if (t && t != VOID_TYPE)
					{
						// implicitly coerce undefined to the return type
						LIns* toplevel = loadToplevel();
						retvalue = callIns(FUNCTIONID(coerce), 3,
							toplevel, retvalue, InsConstPtr(t));
						retvalue = atomToNativeRep(t, retvalue);
					}
				}
				switch (bt(info->returnTraits())) {
				case BUILTIN_number:
					Ins(LIR_fret, retvalue);
					break;
				case BUILTIN_int:
					Ins(LIR_ret, i2p(retvalue));
					break;
				case BUILTIN_uint:
				case BUILTIN_boolean:
					Ins(LIR_ret, u2p(retvalue));
					break;
				default:
					Ins(LIR_ret, retvalue);
					break;
				}
				break;
			}

			case OP_typeof:
			{
                PERFM_NVPROF("emit(unary",1);
				//sp[0] = typeof(sp[0]);
				int32_t index = (int32_t) op1;
				LIns* value = loadAtomRep(index);
				LIns* i3 = callIns(FUNCTIONID(typeof), 2,
					coreAddr, value);
				AvmAssert(result == STRING_TYPE);
				localSet(index, i3);
				break;
			}

			case OP_not:
			{
                PERFM_NVPROF("emit(unary",1);
				int32_t index = (int32_t) op1;
				AvmAssert(state->value(index).traits == BOOLEAN_TYPE);
				LIns* value = localGet(index);
				LIns* i3 = binaryIns(LIR_xor, value, InsConst(1));
				localSet(index, i3);
				break;
			}

            case OP_negate: {
                PERFM_NVPROF("emit(unary",1);
				int32_t index = (int32_t) op1;
				localSet(index, Ins(LIR_fneg, localGetq(index)));
				break;
			}

            case OP_negate_i: {
                PERFM_NVPROF("emit(unary",1);
				//framep[op1] = -framep[op1]
				int32_t index = (int32_t) op1;
				AvmAssert(state->value(index).traits == INT_TYPE);
				localSet(index, Ins(LIR_neg, localGet(index)));
				break;
			}

			case OP_increment:
			case OP_decrement:
			case OP_inclocal:
            case OP_declocal: {
                PERFM_NVPROF("emit(unary",1);
				int32_t index = (int32_t) op1;
				int32_t incr = (int32_t) op2; // 1 or -1
				localSet(index, binaryIns(LIR_fadd, localGetq(index), i2dIns(InsConst(incr))));
				break;
			}

			case OP_inclocal_i:
			case OP_declocal_i:
			case OP_increment_i:
            case OP_decrement_i: {
                PERFM_NVPROF("emit(unary",1);
				int32_t index = (int32_t) op1;
				int32_t incr = (int32_t) op2;
				AvmAssert(state->value(index).traits == INT_TYPE);
				localSet(index, binaryIns(LIR_add, localGet(index), InsConst(incr)));
				break;
			}

            case OP_bitnot: {
                PERFM_NVPROF("emit(unary",1);
				// *sp = core->intToAtom(~integer(*sp));
				int32_t index = (int32_t) op1;
				AvmAssert(state->value(index).traits == INT_TYPE);
				localSet(index, lirout->ins1(LIR_not, localGet(index)));
				break;
			}

            case OP_modulo: {
                PERFM_NVPROF("emit(binary",1);
				LIns* out = callIns(FUNCTIONID(mod), 2,
					localGetq(sp-1), localGetq(sp));
				localSet(sp-1,	out);
				break;
			}

            case OP_divide:
            case OP_multiply:
            case OP_subtract:
            case OP_add_d: {
                PERFM_NVPROF("emit(binary",1);
				LOpcode mircode;
				switch (opcode) {
					default:
					case OP_divide:     mircode = LIR_fdiv; break;
					case OP_multiply:   mircode = LIR_fmul; break;
					case OP_subtract:   mircode = LIR_fsub; break;
					case OP_add_d:      mircode = LIR_fadd; break;
				}
                localSet(sp-1, binaryIns(mircode, localGetq(sp-1), localGetq(sp)));
                break;
            }

			case OP_subtract_i:
			case OP_add_i:
			case OP_multiply_i:
			case OP_lshift:
			case OP_rshift:
			case OP_urshift:
			case OP_bitand:
			case OP_bitor:
			case OP_bitxor:
			{
                PERFM_NVPROF("emit(binary",1);
				LOpcode mircode;
				switch (opcode) {
                    default:
					case OP_bitxor:     mircode = LIR_xor;  break;
					case OP_bitor:      mircode = LIR_or;   break;
					case OP_bitand:     mircode = LIR_and;  break;
					case OP_urshift:    mircode = LIR_ush;  break;
					case OP_rshift:     mircode = LIR_rsh;  break;
					case OP_lshift:     mircode = LIR_lsh;  break;
					case OP_multiply_i: mircode = LIR_mul; break;
					case OP_add_i:      mircode = LIR_add;  break;
					case OP_subtract_i: mircode = LIR_sub;  break;
				}
				LIns* lhs = localGet(sp-1);
				LIns* rhs = localGet(sp);
				LIns* out = binaryIns(mircode, lhs, rhs);
				localSet(sp-1, out);
				break;
			}

			case OP_throw:
			{
                PERFM_NVPROF("emit(throw",1);
				//throwAtom(*sp--);
				int32_t index = (int32_t) op1;
				callIns(FUNCTIONID(throwAtom), 2, coreAddr, loadAtomRep(index));
				break;
			}

			case OP_getsuper:
			{
                PERFM_NVPROF("emit(getsuper",1);
				// stack in: obj [ns [name]]
				// stack out: value
				// sp[0] = env->getsuper(sp[0], multiname)
				int objDisp = sp;
				LIns* multi = initMultiname((Multiname*)op1, objDisp);
				AvmAssert(state->value(objDisp).notNull);

				LIns* obj = loadAtomRep(objDisp);

				LIns* i3 = callIns(FUNCTIONID(getsuper), 3,
					env_param, obj, multi);

				i3 = atomToNativeRep(result, i3);
				localSet(objDisp, i3);
				break;
			}

			case OP_setsuper:
			{
                PERFM_NVPROF("emit(setsuper",1);
				// stack in: obj [ns [name]] value
				// stack out: nothing
				// core->setsuper(sp[-1], multiname, sp[0], env->vtable->base)
				int objDisp = sp-1;
				LIns* multi = initMultiname((Multiname*)op1, objDisp);
				AvmAssert(state->value(objDisp).notNull);

				LIns* obj = loadAtomRep(objDisp);
				LIns* value = loadAtomRep(sp);

				callIns(FUNCTIONID(setsuper), 4,
					env_param, obj, multi, value);

				break;
			}

			case OP_nextname:
			case OP_nextvalue:
			{
                PERFM_NVPROF("emit(next",1);
				// sp[-1] = next[name|value](sp[-1], sp[0]);
				LIns* obj = loadAtomRep(sp-1);
				AvmAssert(state->value(sp).traits == INT_TYPE);
				LIns* index = localGet(sp);
				LIns* i1 = callIns((opcode == OP_nextname) ? FUNCTIONID(nextname) : FUNCTIONID(nextvalue), 3,
                                   env_param, obj, index);
				localSet(sp-1, atomToNativeRep(result, i1));
				break;
			}

			case OP_hasnext: 
			{
                PERFM_NVPROF("emit(hasnext",1);
				// sp[-1] = hasnext(sp[-1], sp[0]);
				LIns* obj = loadAtomRep(sp-1);
				AvmAssert(state->value(sp).traits == INT_TYPE);
				LIns* index = localGet(sp);
				LIns* i1 = callIns(FUNCTIONID(hasnext), 3,
					env_param, obj, index);
				AvmAssert(result == INT_TYPE);
				localSet(sp-1, i1);
				break;
			}

			case OP_hasnext2: 
			{
                PERFM_NVPROF("emit(hasnext2",1);
                // fixme - if obj is already Atom, or index is already int,
                // easier to directly reference space in vars.
				int32_t obj_index = (int32_t) op1;
				int32_t index_index = (int32_t) op2;
				LIns* obj = InsAlloc(sizeof(Atom));
				LIns* index = InsAlloc(sizeof(int32_t));
				storeIns(loadAtomRep(obj_index), 0, obj);		// Atom obj
				storeIns(localGet(index_index), 0, index);   	// int32 index
				LIns* i1 = callIns(FUNCTIONID(hasnextproto), 3,
									 env_param, obj, index);
				localSet(obj_index, loadIns(LIR_ldp, 0, obj));  // Atom obj
				localSet(index_index, loadIns(LIR_ld, 0, index)); // int32 index
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp+1, i1);
				break;
			}
			
			case OP_newfunction:
			{
                PERFM_NVPROF("emit(newfunction",1);
                uint32_t function_id = (uint32_t) op1;
                int32_t index = (int32_t) op2;
				//sp[0] = core->newfunction(env, body, _scopeBase, scopeDepth);
 				AbstractFunction* func = pool->getMethodInfo(function_id);
				int extraScopes = state->scopeDepth;

				// prepare scopechain args for call
				LIns* ap = storeAtomArgs(extraScopes, state->verifier->scopeBase);

				LIns* envArg = env_param;
				LIns* vtable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), envArg);
				LIns* outer = loadIns(LIR_ldcp, offsetof(VTable, scope), vtable);

				LIns* i3 = callIns(FUNCTIONID(newfunction), 4,
					envArg, InsConstPtr(func), outer, ap);

				AvmAssert(!result->isMachineType());
				localSet(index, i3);
				break;
			}

			case OP_call:
			{
                PERFM_NVPROF("emit(call",1);
				// stack in: method obj arg1..N
				// sp[-argc-1] = call(env, sp[-argc], argc, ...)
				int argc = int(op1);
				int funcDisp = sp - argc - 1;
				int dest = funcDisp;

				// convert args to Atom[] for the call
				LIns* func = loadAtomRep(funcDisp);
				LIns* ap = storeAtomArgs(loadAtomRep(funcDisp+1), argc, funcDisp+2);

				LIns* toplevel = loadToplevel();

				LIns* i3 = callIns(FUNCTIONID(op_call), 4, 
					toplevel, func, InsConst(argc), ap);

				localSet(dest, atomToNativeRep(result, i3));
				break;
			}

			case OP_callproperty:
			case OP_callproplex:
			case OP_callpropvoid:
			{
                PERFM_NVPROF("emit(callprop",1);
				// stack in: obj [ns [name]] arg1..N
				// stack out: result

				int argc = int(op2);
				// obj = sp[-argc]
				//tempAtom = callproperty(env, name, toVTable(obj), argc, ...);
				//	*(sp -= argc) = tempAtom;
				int argv = sp-argc+1;

				int baseDisp = sp-argc;
				LIns* multi = initMultiname((Multiname*)op1, baseDisp);

				AvmAssert(state->value(baseDisp).notNull);

				// convert args to Atom[] for the call
				LIns* base = loadAtomRep(baseDisp);
				LIns* receiver = opcode == OP_callproplex ? InsConstAtom(nullObjectAtom) : base;
				LIns* ap = storeAtomArgs(receiver, argc, argv);

				LIns* vtable = loadVTable(baseDisp);
				LIns* toplevel = loadToplevel();

				LIns* out = callIns(FUNCTIONID(callproperty), 6,
					toplevel, base, multi, InsConst(argc), ap, vtable);

				localSet(baseDisp, atomToNativeRep(result, out));
				break;
			}

			case OP_constructprop:
			{
                PERFM_NVPROF("emit(constructprop",1);
				// stack in: obj [ns [name]] arg1..N
				// stack out: result

				int argc = int(op2);
				// obj = sp[-argc]
				//tempAtom = callproperty(env, name, toVTable(obj), argc, ...);
				//	*(sp -= argc) = tempAtom;
				int argv = sp-argc+1;

				int objDisp = sp-argc;
				LIns* multi = initMultiname((Multiname*)op1, objDisp);
				AvmAssert(state->value(objDisp).notNull);

				// convert args to Atom[] for the call
				LIns* obj = loadAtomRep(objDisp);
				LIns* vtable = loadVTable(objDisp);
				LIns* ap = storeAtomArgs(obj, argc, argv);

				LIns* toplevel = loadToplevel();
				LIns* i3 = callIns(FUNCTIONID(constructprop), 5,
					toplevel, multi, InsConst(argc), ap, vtable);

				localSet(objDisp, atomToNativeRep(result, i3));
				break;
			}

			case OP_callsuper:
			case OP_callsupervoid:
			{
                PERFM_NVPROF("emit(callsuper",1);
				// stack in: obj [ns [name]] arg1..N
				// stack out: result
				// null check must have already happened.
				//	tempAtom = callsuper(multiname, obj, sp-argc+1, argc, vtable->base);
				int argc = int(op2);
				int argv = sp - argc + 1;
				int objDisp = sp - argc;
				LIns* multi = initMultiname((Multiname*)op1, objDisp);
				AvmAssert(state->value(objDisp).notNull);

				// convert args to Atom[] for the call
				LIns* obj = loadAtomRep(objDisp);

				LIns* ap = storeAtomArgs(obj, argc, argv);

				LIns* i3 = callIns(FUNCTIONID(callsuper), 4,
					env_param, multi, InsConst(argc), ap);

				localSet(objDisp, atomToNativeRep(result, i3));
				break;
			}

			case OP_construct:
 			{
                PERFM_NVPROF("emit(construct",1);
				// stack in: method arg1..N
				// sp[-argc] = construct(env, sp[-argc], argc, null, arg1..N)
 				int argc = int(op1);
 				int funcDisp = sp - argc;
 				int dest = funcDisp;

				LIns* func = loadAtomRep(funcDisp);

				// convert args to Atom[] for the call
				LIns* ap = storeAtomArgs(InsConstAtom(nullObjectAtom), argc, funcDisp+1);

				LIns* toplevel = loadToplevel();

				LIns* i3 = callIns(FUNCTIONID(op_construct), 4,
					toplevel, func, InsConst(argc), ap);

				localSet(dest, atomToNativeRep(result, i3));
				break;
 			}

			case OP_applytype:
			{
                PERFM_NVPROF("emit(applytype",1);
				// stack in: method arg1..N
				// sp[-argc] = applytype(env, sp[-argc], argc, null, arg1..N)
				int argc = int(op1);
				int funcDisp = sp - argc;
				int dest = funcDisp;
				int arg0 = sp - argc + 1;

				LIns* func = loadAtomRep(funcDisp);

				// convert args to Atom[] for the call
				LIns* ap = storeAtomArgs(argc, arg0);

				LIns* toplevel = loadToplevel();

				LIns* i3 = callIns(FUNCTIONID(op_applytype), 4,
					toplevel, func, InsConst(argc), ap);

				localSet(dest, atomToNativeRep(result, i3));
				break;
			}

			case OP_newobject:
			{
                PERFM_NVPROF("emit(newobject",1);
 				// result = env->op_newobject(sp, argc)
 				int argc = int(op1);
 				int dest = sp - (2*argc-1);
				int arg0 = dest;

				// convert args to Atom for the call[]
				LIns* ap = storeAtomArgs(2*argc, arg0);

				LIns* i3 = callIns(FUNCTIONID(op_newobject), 3,
					env_param, leaIns(sizeof(Atom)*(2*argc-1), ap), InsConst(argc));

				localSet(dest, ptrToNativeRep(result, i3));
				break;
			}

			case OP_newactivation:
			{
                PERFM_NVPROF("emit(newactivation",1);
 				// result = env->newActivation()
				LIns* activation = callIns(FUNCTIONID(newActivation), 1, env_param);
				localSet(sp+1, ptrToNativeRep(result, activation));
				break;
			}

			case OP_newcatch:
			{
                PERFM_NVPROF("emit(newcatch",1);
 				// result = core->newObject(env->activation, NULL);
 				int dest = sp+1;

				LIns* activation = callIns(FUNCTIONID(newcatch), 2, 
										 env_param, InsConstPtr(result));

				localSet(dest, ptrToNativeRep(result, activation));
				break;
			}

 			case OP_newarray:
 			{
                PERFM_NVPROF("emit(newarray",1);
				// sp[-argc+1] = core->arrayClass->newarray(sp-argc+1, argc)
 				int argc = int(op1);
 				int arg0 = sp - 1*argc+1;

				// convert array elements to Atom[]
				LIns* ap = storeAtomArgs(argc, arg0);
				LIns* toplevel = loadToplevel();
				LIns* arrayClass = loadIns(LIR_ldcp, offsetof(Toplevel,arrayClass), toplevel);
				LIns* i3 = callIns(FUNCTIONID(newarray), 3,
					arrayClass, ap, InsConst(argc));

				AvmAssert(!result->isMachineType());
				localSet(arg0, i3);
				break;
 			}

			case OP_newclass:
			{
                PERFM_NVPROF("emit(newclass",1);
				// sp[0] = core->newclass(env, cinit, scopeBase, scopeDepth, base)
				AbstractFunction *cinit = (AbstractFunction*) op1;
				int localindex = int(op2);
				int extraScopes = state->scopeDepth;

				LIns* envArg = env_param;
				LIns* vtable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), envArg);
				LIns* outer = loadIns(LIR_ldcp, offsetof(VTable, scope), vtable);
				LIns* base = localGetp(localindex);

				// prepare scopechain args for call
				LIns* ap = storeAtomArgs(extraScopes, state->verifier->scopeBase);

				LIns* i3 = callIns(FUNCTIONID(newclass), 5, 
					envArg, InsConstPtr(cinit), base, outer, ap);

				AvmAssert(!result->isMachineType());
				localSet(localindex, i3);
				break;
			}

			case OP_getdescendants:
			{
                PERFM_NVPROF("emit(getdesc",1);
				// stack in: obj [ns [name]]
				// stack out: value
				//sp[0] = core->getdescendants(sp[0], name);
				int objDisp = sp;
				Multiname* multiname = (Multiname*) op1;

				LIns* envArg = env_param;
				LIns* out;
				LIns* multi = initMultiname(multiname, objDisp);
				LIns* obj = loadAtomRep(objDisp);
				AvmAssert(state->value(objDisp).notNull);

				out = callIns(FUNCTIONID(getdescendants), 3,
					envArg, obj, multi);

				localSet(objDisp, atomToNativeRep(result, out));
				break;
			}

            case OP_checkfilter: {
                PERFM_NVPROF("emit(checkfilter",1);
				int32_t index = (int32_t) op1;
				callIns(FUNCTIONID(checkfilter), 2, env_param, loadAtomRep(index));
				break;
			}

			case OP_findpropstrict: 
			case OP_findproperty: 
			{
                PERFM_NVPROF("emit(findprop",1);
				// stack in: [ns [name]]
				// stack out: obj
				// sp[1] = env->findproperty(scopeBase, scopedepth, name, strict)
				int dest = sp;
				LIns* multi = initMultiname((Multiname*)op1, dest);
				dest++;
				int extraScopes = state->scopeDepth;

				// prepare scopechain args for call
				LIns* ap = storeAtomArgs(extraScopes, state->verifier->scopeBase);

				LIns* envArg = env_param;
				LIns* vtable = loadIns(LIR_ldcp, offsetof(MethodEnv,vtable), envArg);
				LIns* outer = loadIns(LIR_ldcp, offsetof(VTable,scope), vtable);

				LIns* withBase;
				if (state->withBase == -1)
				{
					withBase = InsConstPtr(0);
				}
				else
				{
					withBase = leaIns(state->withBase*sizeof(Atom), ap);
				}
	
				// 		return env->findproperty(outer, argv, extraScopes, name, strict);

				LIns* i3 = callIns(FUNCTIONID(findproperty), 7, 
					envArg, outer, ap, InsConst(extraScopes), multi, 
					InsConst((int32_t)(opcode == OP_findpropstrict)),
					withBase);

				localSet(dest, atomToNativeRep(result, i3));
				break;
			}

			case OP_finddef: 
			{
                PERFM_NVPROF("emit(finddef",1);
				// stack in: 
				// stack out: obj
				// framep[op2] = env->finddef(name)
				Multiname* multiname = (Multiname*) op1;
				int32_t dest_index = (int32_t) op2;
				LIns* name = InsConstPtr(multiname->getName());
				LIns* out;

				AvmAssert(multiname->isBinding());
				if (multiname->isNsset())
				{
					out = callIns(FUNCTIONID(finddefNsset), 3,
						env_param,
						InsConstPtr(multiname->getNsset()),
						name);
				}
				else
				{
					out = callIns(FUNCTIONID(finddefNs), 3,
						env_param,
						InsConstPtr(multiname->getNamespace()),
						name);
				}
				localSet(dest_index, ptrToNativeRep(result, out));
				break;
			}

			case OP_getproperty:
			{
                PERFM_NVPROF("emit(getproperty",1);
				// stack in: obj [ns] [name]
				// stack out: value
				// obj=sp[0]
				//sp[0] = env->getproperty(obj, multiname);

				Multiname* multiname = (Multiname*)op1;
				bool attr = multiname->isAttr();
				Traits* indexType = state->value(sp).traits;
				int objDisp = sp;

				bool maybeIntegerIndex = !attr && multiname->isRtname() && multiname->contains(core->publicNamespace);

				if (maybeIntegerIndex && indexType == INT_TYPE)
				{
					bool valIsAtom = true;
					LIns* index = localGet(objDisp--);
					
					if (multiname->isRtns())
					{
						// Discard runtime namespace
						objDisp--;
					}
					
					Traits* objType = state->value(objDisp).traits;

					LIns *value;
					if (objType == ARRAY_TYPE || (objType!= NULL && objType->containsInterface(VECTOROBJ_TYPE)) )
					{
						value = callIns((objType==ARRAY_TYPE ? 
										FUNCTIONID(ArrayObject_getIntProperty) :
										FUNCTIONID(ObjectVectorObject_getIntProperty)), 2,
							localGetp(sp-1), index);
					}
					else if( objType == VECTORINT_TYPE || objType == VECTORUINT_TYPE)
					{
						if( result == INT_TYPE || result == UINT_TYPE)
						{
							value = callIns((objType==VECTORINT_TYPE ? 
													FUNCTIONID(IntVectorObject_getNativeIntProperty) :
													FUNCTIONID(UIntVectorObject_getNativeIntProperty)), 2,
							localGetp(sp-1), index);
							valIsAtom = false;
						}
						else
						{
							value = callIns((objType==VECTORINT_TYPE ? 
													FUNCTIONID(IntVectorObject_getIntProperty) :
													FUNCTIONID(UIntVectorObject_getIntProperty)), 2,
							localGetp(sp-1), index);
						}
					}
					else if( objType == VECTORDOUBLE_TYPE )
					{
						if( result == NUMBER_TYPE )
						{
							value = callIns(FUNCTIONID(DoubleVectorObject_getNativeIntProperty), 2,
								localGetp(sp-1), index);
							valIsAtom = false;
						}
						else
						{
							value = callIns(FUNCTIONID(DoubleVectorObject_getIntProperty), 2,
								localGetp(sp-1), index);
						}
					}
					else
					{
						value = callIns(FUNCTIONID(getpropertylate_i), 3,
							env_param, loadAtomRep(sp-1), index);
					}

					localSet(sp-1, valIsAtom?atomToNativeRep(result, value):value);
				}
				else if (maybeIntegerIndex && indexType == UINT_TYPE)
				{
					bool valIsAtom = true;

					LIns* index = localGet(objDisp--);
					
					if (multiname->isRtns())
					{
						// Discard runtime namespace
						objDisp--;
					}
					
					Traits* objType = state->value(objDisp).traits;

					LIns *value;
					if (objType == ARRAY_TYPE || (objType!= NULL && objType->containsInterface(VECTOROBJ_TYPE)))
					{
						value = callIns((objType==ARRAY_TYPE ? 
												FUNCTIONID(ArrayObject_getUintProperty) :
												FUNCTIONID(ObjectVectorObject_getUintProperty)), 2,
							localGetp(sp-1), index);
					}
					else if( objType == VECTORINT_TYPE || objType == VECTORUINT_TYPE )
					{
						if( result == INT_TYPE || result == UINT_TYPE )
						{
							value = callIns((objType==VECTORINT_TYPE ?
													FUNCTIONID(IntVectorObject_getNativeUintProperty) :
													FUNCTIONID(UIntVectorObject_getNativeUintProperty)), 2,
							localGetp(sp-1), index);
							valIsAtom = false;
						}
						else
						{
							value = callIns((objType==VECTORINT_TYPE ?
													FUNCTIONID(IntVectorObject_getUintProperty) : 
													FUNCTIONID(UIntVectorObject_getUintProperty)), 2,
							localGetp(sp-1), index);
						}
					}
					else if( objType == VECTORDOUBLE_TYPE )
					{
						if( result == NUMBER_TYPE )//|| result == UINT_TYPE)
						{
							value = callIns(FUNCTIONID(DoubleVectorObject_getNativeUintProperty), 2,
								localGetp(sp-1), index);
							valIsAtom = false;
						}
						else
						{
							value = callIns(FUNCTIONID(DoubleVectorObject_getUintProperty), 2,
								localGetp(sp-1), index);
						}
					}
					else
					{
						value = callIns(FUNCTIONID(getpropertylate_u), 3,
							env_param, loadAtomRep(sp-1), index);
					}

					localSet(sp-1, valIsAtom?atomToNativeRep(result, value):value);
				}
				else if (maybeIntegerIndex && indexType != STRING_TYPE)
				{
					LIns* _tempname = InsAlloc(sizeof(Multiname));

					// copy the flags
					LIns* mFlag = InsConst(multiname->ctFlags());
					storeIns(mFlag, offsetof(Multiname,flags), _tempname);

					LIns* index = loadAtomRep(objDisp--);
					AvmAssert(state->value(objDisp).notNull);

					LIns* obj = loadAtomRep(objDisp);

					// copy the compile-time namespace to the temp multiname
					LIns* mSpace = InsConstPtr(multiname->ns);
					storeIns(mSpace, offsetof(Multiname, ns), _tempname);

					LIns* value = callIns(FUNCTIONID(getpropertyHelper), 5,
									    env_param, obj, _tempname, loadVTable(objDisp), index);

					localSet(objDisp, atomToNativeRep(result, value));
				}
				else
				{
					LIns* multi = initMultiname((Multiname*)op1, objDisp);
					AvmAssert(state->value(objDisp).notNull);

					LIns* vtable = loadVTable(objDisp);
					LIns* obj = loadAtomRep(objDisp);
					LIns* toplevel = loadToplevel();

					//return toplevel->getproperty(obj, name, toplevel->toVTable(obj));
					LIns* value = callIns(FUNCTIONID(getproperty), 4,
										toplevel, obj, multi, vtable);

					localSet(objDisp, atomToNativeRep(result, value));
				}
				break;
			}

			case OP_initproperty:
			case OP_setproperty:
			{
                PERFM_NVPROF("emit(setproperty",1);
				// stack in: obj [ns] [name] value
				// stack out:
				// obj = sp[-1]
				//env->setproperty(obj, multiname, sp[0], toVTable(obj));
				//LIns* value = loadAtomRep(sp);

				Multiname* multiname = (Multiname*)op1;
				bool attr = multiname->isAttr();
				Traits* indexType = state->value(sp-1).traits;
				Traits* valueType = state->value(sp).traits;
				int objDisp = sp-1;

				bool maybeIntegerIndex = !attr && multiname->isRtname() && multiname->contains(core->publicNamespace);

				if (maybeIntegerIndex && indexType == INT_TYPE)
				{
					LIns* index = localGet(objDisp--);
					
					if (multiname->isRtns())
					{
						// Discard runtime namespace
						objDisp--;
					}
					
					Traits* objType = state->value(objDisp).traits;

					if (objType == ARRAY_TYPE || (objType!= NULL && objType->containsInterface(VECTOROBJ_TYPE)))
					{
						LIns* value = loadAtomRep(sp);
						callIns((objType==ARRAY_TYPE ? 
										FUNCTIONID(ArrayObject_setIntProperty) :
										FUNCTIONID(ObjectVectorObject_setIntProperty)), 3,
							localGetp(objDisp), index, value);
					}
					else if(objType == VECTORINT_TYPE || objType == VECTORUINT_TYPE )
					{
						if( valueType == INT_TYPE )
						{
							LIns* value = localGet(sp);
							callIns((objType==VECTORINT_TYPE ? 
											FUNCTIONID(IntVectorObject_setNativeIntProperty) :
											FUNCTIONID(UIntVectorObject_setNativeIntProperty)),
											3,
											localGetp(objDisp), index, value);
						}
						else if( valueType == UINT_TYPE )
						{
							LIns* value = localGet(sp);
							callIns((objType==VECTORINT_TYPE ? 
											FUNCTIONID(IntVectorObject_setNativeIntProperty) :
											FUNCTIONID(UIntVectorObject_setNativeIntProperty)), 
											3,
											localGetp(objDisp), index, value);
						}
						else
						{
							LIns* value = loadAtomRep(sp);
							value = callIns((objType==VECTORINT_TYPE ? 
													FUNCTIONID(IntVectorObject_setIntProperty) :
													FUNCTIONID(UIntVectorObject_setIntProperty)), 
													3,
													localGetp(objDisp), index, value);
						}
					}
					else if(objType == VECTORDOUBLE_TYPE)
					{
						if( valueType == NUMBER_TYPE )
						{
							LIns* value = localGetq(sp);
							callIns(FUNCTIONID(DoubleVectorObject_setNativeIntProperty), 3,
								localGetp(objDisp), index, value);
						}
						else
						{
							LIns* value = loadAtomRep(sp);
							value = callIns(FUNCTIONID(DoubleVectorObject_setIntProperty), 3,
								localGetp(objDisp), index, value);
						}
					}
					else
					{
						LIns* value = loadAtomRep(sp);
						callIns(FUNCTIONID(setpropertylate_i), 4,
							env_param, loadAtomRep(objDisp), index, value);
					}
				}
				else if (maybeIntegerIndex && indexType == UINT_TYPE)
				{
					LIns* index = localGet(objDisp--);
					
					if (multiname->isRtns())
					{
						// Discard runtime namespace
						objDisp--;
					}
					
					Traits* objType = state->value(objDisp).traits;

					if (objType == ARRAY_TYPE || (objType!= NULL && objType->containsInterface(VECTOROBJ_TYPE)))
					{
						LIns* value = loadAtomRep(sp);
						callIns((objType==ARRAY_TYPE ? 
										FUNCTIONID(ArrayObject_setUintProperty) :
										FUNCTIONID(ObjectVectorObject_setUintProperty)), 3,
							localGetp(objDisp), index, value);
					}
					else if(objType == VECTORINT_TYPE || objType == VECTORUINT_TYPE )
					{
						if( valueType == INT_TYPE )
						{
							LIns* value = localGet(sp);
							callIns((objType==VECTORINT_TYPE ? 
											FUNCTIONID(IntVectorObject_setNativeUintProperty) :
											FUNCTIONID(UIntVectorObject_setNativeUintProperty)),
											3,
											localGetp(objDisp), index, value);
						}
						else if( valueType == UINT_TYPE )
						{
							LIns* value = localGet(sp);
							callIns((objType==VECTORINT_TYPE ? 
											FUNCTIONID(IntVectorObject_setNativeUintProperty) :
											FUNCTIONID(UIntVectorObject_setNativeUintProperty)), 
											3,
											localGetp(objDisp), index, value);
						}
						else
						{
							LIns* value = loadAtomRep(sp);
							value = callIns((objType==VECTORINT_TYPE ? 
													FUNCTIONID(IntVectorObject_setUintProperty) :
													FUNCTIONID(UIntVectorObject_setUintProperty)), 
													3,
													localGetp(objDisp), index, value);
						}
					}
					else if(objType == VECTORDOUBLE_TYPE)
					{
						if( valueType == NUMBER_TYPE )
						{
							LIns* value = localGetq(sp);
							callIns(FUNCTIONID(DoubleVectorObject_setNativeUintProperty), 3,
								localGetp(objDisp), index, value);
						}
						else
						{
							LIns* value = loadAtomRep(sp);
							value = callIns(FUNCTIONID(DoubleVectorObject_setUintProperty), 3,
								localGetp(objDisp), index, value);
						}
					}
					else
					{
						LIns* value = loadAtomRep(sp);
						callIns(FUNCTIONID(setpropertylate_u), 4,
							env_param, loadAtomRep(objDisp), index, value);
					}
				}
				else if (maybeIntegerIndex)
				{
					LIns* value = loadAtomRep(sp);
					LIns* _tempname = InsAlloc(sizeof(Multiname));

					// copy the flags
					LIns* mFlag = InsConst(multiname->ctFlags());
					storeIns(mFlag, offsetof(Multiname,flags), _tempname);

					LIns* index = loadAtomRep(objDisp--);
					AvmAssert(state->value(objDisp).notNull);

					LIns* vtable = loadVTable(objDisp);
					LIns* obj = loadAtomRep(objDisp);
					LIns* envarg = env_param;

					// copy the compile-time namespace to the temp multiname
					LIns* mSpace = InsConstPtr(multiname->ns);
					storeIns(mSpace, offsetof(Multiname, ns), _tempname);

					const CallInfo *func = opcode==OP_setproperty ? FUNCTIONID(setpropertyHelper) :
														FUNCTIONID(initpropertyHelper);
					callIns(func, 6, envarg, obj, _tempname, value, vtable, index);

					localSet(objDisp, atomToNativeRep(result, value));
				}
				else
				{
					LIns* value = loadAtomRep(sp);
					LIns* multi = initMultiname((Multiname*)op1, objDisp);
					AvmAssert(state->value(objDisp).notNull);

					LIns* vtable = loadVTable(objDisp);
					LIns* obj = loadAtomRep(objDisp);

					if (OP_setproperty)
					{
						LIns* toplevel = loadToplevel();
						callIns(FUNCTIONID(setproperty), 5,
										toplevel, obj, multi, value, vtable);
					}
					else
					{
						callIns(FUNCTIONID(initproperty), 5,
							env_param, obj, multi, value, vtable);
					}
				}
				break;
			}

			case OP_deleteproperty:
			{
                PERFM_NVPROF("emit(delproperty",1);
				// stack in: obj [ns] [name]
				// stack out: Boolean
				//sp[0] = delproperty(sp[0], multiname);
				int objDisp = sp;
				Multiname *multiname = (Multiname*)op1;
				if(!multiname->isRtname()) {
					LIns* multi = initMultiname(multiname, objDisp, true);

					LIns* obj = loadAtomRep(objDisp);
					
					LIns* i3 = callIns(FUNCTIONID(delproperty), 3,
						env_param, obj, multi);

					localSet(objDisp, atomToNativeRep(result, i3));
				} else {
					LIns* _tempname = InsAlloc(sizeof(Multiname));

					// copy the flags
					LIns* mFlag = InsConst(multiname->ctFlags());
					storeIns(mFlag, offsetof(Multiname,flags), _tempname);

					LIns* index = loadAtomRep(objDisp--);

					if( !multiname->isRtns() )
					{
						// copy the compile-time namespace to the temp multiname
						LIns* mSpace = InsConstPtr(multiname->ns);
						storeIns(mSpace, offsetof(Multiname, ns), _tempname);
					}
					else
					{
						// intern the runtime namespace and copy to the temp multiname
						LIns* nsAtom = loadAtomRep(objDisp--);
						LIns* internNs = callIns(FUNCTIONID(internRtns), 2,
							env_param, nsAtom);

						storeIns(internNs, offsetof(Multiname,ns), _tempname);
					}

					AvmAssert(state->value(objDisp).notNull);
					LIns* obj = loadAtomRep(objDisp);

					LIns* value = callIns(FUNCTIONID(delpropertyHelper), 4,
									    env_param, obj, _tempname, index);

					localSet(objDisp, atomToNativeRep(result, value));
				}
				break;
			}

			case OP_convert_s:
			{
                PERFM_NVPROF("emit(unary",1);
				int32_t index = (int32_t) op1;
				localSet(index, callIns(FUNCTIONID(string), 2,
					coreAddr, loadAtomRep(index)));
				break;
			}

			case OP_esc_xelem: // ToXMLString will call EscapeElementValue
			{
                PERFM_NVPROF("emit(unary",1);
				//sp[0] = core->ToXMLString(sp[0]);
				int32_t index = (int32_t) op1;
				LIns* value = loadAtomRep(index);
				LIns* i3 = callIns(FUNCTIONID(ToXMLString), 2,
					coreAddr, value);
				AvmAssert(result == STRING_TYPE);
				localSet(index, i3);
				break;
			}

			case OP_esc_xattr:
			{
                PERFM_NVPROF("emit(unary",1);
				//sp[0] = core->EscapeAttributeValue(sp[0]);
				int32_t index = (int32_t) op1;
				LIns* value = loadAtomRep(index);
				LIns* i3 = callIns(FUNCTIONID(EscapeAttributeValue), 2,
					coreAddr, value);
				AvmAssert(result == STRING_TYPE);
				localSet(index, i3);
				break;
			}

			case OP_astype:
			{
                PERFM_NVPROF("emit(astype",1);
				// sp[0] = core->astype(sp[0], traits)
				Traits *type = (Traits*) op1;
				int32_t index = (int32_t) op2;
				LIns* obj = loadAtomRep(index);
				LIns* i1 = callIns(FUNCTIONID(astype), 3,
					coreAddr,
					obj,
					InsConstPtr(type));

				i1 = atomToNativeRep(result, i1);
				localSet(index, i1);
				break;
			}

			case OP_astypelate:
			{
                PERFM_NVPROF("emit(astypelate",1);
				//sp[-1] = astype(sp[-1], toClassITraits(sp[0]));
				//sp--;
				LIns* type = loadAtomRep(sp);

				LIns* itraits = callIns(FUNCTIONID(toClassITraits), 2,
					loadToplevel(), type);

				LIns* obj = loadAtomRep(sp-1);

				LIns* i3 = callIns(FUNCTIONID(astype), 3,
					coreAddr, obj, itraits);

				i3 = atomToNativeRep(result, i3);
				localSet(sp-1, i3);
				break;
			}


			case OP_add:
			{
                PERFM_NVPROF("emit(binary",1);
				LIns* lhs = loadAtomRep(sp-1);
				LIns* rhs = loadAtomRep(sp);
				LIns* toplevel = loadToplevel();
				LIns* out = callIns(FUNCTIONID(add2), 3,
					toplevel, lhs, rhs);
				localSet(sp-1, atomToNativeRep(result, out));
				break;
			}

			case OP_concat:
			{
                PERFM_NVPROF("emit(binary",1);
				LIns* lhs = localGetp(sp-1);
				LIns* rhs = localGetp(sp);
				LIns* out = callIns(FUNCTIONID(concatStrings), 3,
					coreAddr, lhs, rhs);
				localSet(sp-1,	out);
				break;
			}

			case OP_strictequals:
			{
                PERFM_NVPROF("emit(compare",1);
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp-1, cmpEq(FUNCTIONID(stricteq), sp-1, sp));
				break;
			}

			case OP_equals:
			{
                PERFM_NVPROF("emit(compare",1);
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp-1, cmpEq(FUNCTIONID(equals), sp-1, sp));
				break;
			}

			case OP_lessthan:
			{
                PERFM_NVPROF("emit(compare",1);
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp-1, cmpLt(sp-1, sp));
				break;
			}

			case OP_lessequals:
			{
                PERFM_NVPROF("emit(compare",1);
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp-1, cmpLe(sp-1, sp));
				break;
			}

			case OP_greaterthan:
			{
                PERFM_NVPROF("emit(compare",1);
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp-1, cmpLt(sp, sp-1));
				break;
			}

			case OP_greaterequals: 
			{
                PERFM_NVPROF("emit(compare",1);
				AvmAssert(result == BOOLEAN_TYPE);
				localSet(sp-1, cmpLe(sp, sp-1));
				break;
			}

			case OP_instanceof:
			{
                PERFM_NVPROF("emit(instanceof",1);
				LIns* lhs = loadAtomRep(sp-1);
				LIns* rhs = loadAtomRep(sp);
				LIns* toplevel = loadToplevel();
				LIns* out = callIns(FUNCTIONID(instanceof), 3,
					toplevel, lhs, rhs);
				out = atomToNativeRep(result, out);
				localSet(sp-1,	out);
				break;
			}

			case OP_in:
			{
                PERFM_NVPROF("emit(in",1);
				LIns* lhs = loadAtomRep(sp-1);
				LIns* rhs = loadAtomRep(sp);
				LIns* toplevel = loadToplevel();
				LIns* out = callIns(FUNCTIONID(in), 3,
					toplevel, lhs, rhs);
				out = atomToNativeRep(result, out);
				localSet(sp-1, out);
				break;
			}

			case OP_istype:
			{			
                PERFM_NVPROF("emit(istype",1);
				// expects a CONSTANT_Multiname cpool index
				// used when operator "is" RHS is a compile-time type constant
				//sp[0] = istype(sp[0], itraits);
                Traits *type = (Traits*) op1;
                int32_t index = (int32_t) op2;
				LIns* obj = loadAtomRep(index);
				LIns* itraits = InsConstPtr(type);
				LIns* out = callIns(FUNCTIONID(istypeAtom), 3,
					coreAddr, obj, itraits);
				out = atomToNativeRep(result, out);
				localSet(index, out);
				break;
			}

			case OP_istypelate:
			{
                PERFM_NVPROF("emit(istypelate",1);
				//sp[-1] = istype(sp[-1], toClassITraits(sp[0]));
				//sp--;
				LIns* type = loadAtomRep(sp);

				LIns* traits = callIns(FUNCTIONID(toClassITraits), 2,
					loadToplevel(), type);

				LIns* obj = loadAtomRep(sp-1);

				LIns* i3 = callIns(FUNCTIONID(istypeAtom), 3,
					coreAddr, obj, traits);

				i3 = atomToNativeRep(result, i3);
				localSet(sp-1, i3);
				break;
			}

			case OP_dxns:
			{
                PERFM_NVPROF("emit(dxns",1);
				LIns* uri = InsConstPtr((String*)op1); // uri
				LIns* ns = callIns(FUNCTIONID(newPublicNamespace), 
					2, 
					coreAddr, 
					uri);
				storeIns(ns, 0, dxns);
				break;
			}

			case OP_dxnslate:
			{
                PERFM_NVPROF("emit(dxnslate",1);
				int32_t index = (int32_t) op1;
				LIns* atom = loadAtomRep(index);				
				LIns* uri = callIns(FUNCTIONID(intern), 2,
					coreAddr, atom);
				LIns* ns = callIns(FUNCTIONID(newPublicNamespace), 
					2, 
					coreAddr, 
					uri);
				storeIns(ns, 0, dxns);
				break;
			}

			/*
			 * debugger instructions 
			 */
			case OP_debugfile:
			{
                PERFM_NVPROF("emit(debugfile",1);
			#ifdef DEBUGGER
			if (core->debugger())
			{
				// todo refactor api's so we don't have to pass argv/argc
				LIns* debugger = loadIns(LIR_ldcp, offsetof(AvmCore, _debugger),
											coreAddr);
				callIns(FUNCTIONID(debugFile), 2,
						debugger,
						InsConstPtr((String*)op1));
			}
			#endif // DEBUGGER
           #ifdef VTUNE
				Ins(LIR_file, InsConstPtr((String*)op1));
           #endif /* VTUNE */
				break;
		    }

			case OP_debugline:
			{
                PERFM_NVPROF("emit(debugline",1);
			#ifdef DEBUGGER
			if (core->debugger())
			{
				// todo refactor api's so we don't have to pass argv/argc
				LIns* debugger = loadIns(LIR_ldcp, offsetof(AvmCore, _debugger),
											coreAddr);
				callIns(FUNCTIONID(debugLine), 2,
						debugger,
						InsConst((int32_t)op1));
			}
			#endif // DEBUGGER
			#ifdef VTUNE
				Ins(LIR_line, InsConst((int32_t)op1));
				hasDebugInfo = true;
           #endif /* VTUNE */
				break;
            }

			default:
			{
				AvmAssert(false); // unsupported
			}
		}

	} // emit()

	void CodegenLIR::opcodeVerified(AbcOpcode opcode, FrameState* state)
	{
		// flush the buffer after each opcode is processed.
		(void)opcode;
		(void) state;
		verbose_only( if (vbWriter) { vbWriter->flush();} )
	}

	void CodegenLIR::emitIf(FrameState *state, AbcOpcode opcode, intptr_t target, int a, int b)
	{
		if (outOMem()) return;
		this->state = state;

#ifdef DEBUGGER
		Sampler* s = core->get_sampler();
		if (s && s->sampling() && target < state->pc)
		{
			emitSampleCheck();
		}
#endif

		// 
		// compile instructions that cannot throw exceptions before we add exception handling logic
		//

		// op1 = abc opcode target
		// op2 = what local var contains condition

		LIns* cond;
		LOpcode br;
		
		switch (opcode)
		{
		case OP_iftrue:
			NanoAssert(state->value(a).traits == BOOLEAN_TYPE);
			br = LIR_jf;
			cond = binaryIns(LIR_eq, localGet(a), InsConst(0));
			break;
		case OP_iffalse:
			NanoAssert(state->value(a).traits == BOOLEAN_TYPE);
			br = LIR_jt;
			cond = binaryIns(LIR_eq, localGet(a), InsConst(0));
			break;
		case OP_iflt:
			br = LIR_jt;
			cond = cmpLt(a, b);
			break;
		case OP_ifnlt:
			br = LIR_jf;
			cond = cmpLt(a, b);
			break;
		case OP_ifle:
			br = LIR_jt;
			cond = cmpLe(a, b);
			break;
		case OP_ifnle:
			br = LIR_jf;
			cond = cmpLe(a, b);
			break;
		case OP_ifgt:  // a>b === b<a
			br = LIR_jt;
			cond = cmpLt(b, a);
			break;
		case OP_ifngt: // !(a>b) === !(b<a)
			br = LIR_jf;
			cond = cmpLt(b, a);
			break;
		case OP_ifge:  // a>=b === b<=a
			br = LIR_jt;
			cond = cmpLe(b, a);
			break;
		case OP_ifnge: // !(a>=b) === !(a<=b)
			br = LIR_jf;
			cond = cmpLe(b, a);
			break;
		case OP_ifeq:
			br = LIR_jt;
			cond = cmpEq(FUNCTIONID(equals), a, b);
			break;
		case OP_ifne:
			br = LIR_jf;
			cond = cmpEq(FUNCTIONID(equals), a, b);
			break;
		case OP_ifstricteq:
			br = LIR_jt;
			cond = cmpEq(FUNCTIONID(stricteq), a, b);
			break;
		case OP_ifstrictne:
			br = LIR_jf;
			cond = cmpEq(FUNCTIONID(stricteq), a, b);
			break;
		default:
			AvmAssert(false);
			return;
		}

        if (cond->isconst()) {
            if (br == LIR_jt && cond->constval() || br == LIR_jf && !cond->constval()) {
                // taken
                br = LIR_j;
                cond = 0;
            }
            else {
                // not taken = no-op
                return;
            }
        }

        branchIns(br, cond, target);
	} // emitIf()
	
	// Faster compares for ints, uint, doubles
	LIns* CodegenLIR::cmpOptimization(int lhsi, int rhsi, LOpcode icmp, LOpcode ucmp, LOpcode fcmp)
	{
		Traits* lht = state->value(lhsi).traits;
		Traits* rht = state->value(rhsi).traits;

		if (lht == rht && lht == INT_TYPE)
		{
			LIns* lhs = localGet(lhsi);
			LIns* rhs = localGet(rhsi);
			return binaryIns(icmp, lhs, rhs);
		}
		else if (lht == rht && lht == UINT_TYPE)
		{
			LIns* lhs = localGet(lhsi);
			LIns* rhs = localGet(rhsi);
			return binaryIns(ucmp, lhs, rhs);
		}
		else if (lht && lht->isNumeric() && rht && rht->isNumeric())
		{
			// If we're comparing a uint to an int and the int is a non-negative
			// integer constant, don't promote to doubles for the compare
			if ((lht == UINT_TYPE) && (rht == INT_TYPE))
			{
				LIns* lhs = localGet(lhsi);
				LIns* rhs = localGet(rhsi);
			#ifdef AVMPLUS_64BIT
				// 32-bit signed and unsigned values fit in 64-bit registers
				// so we can promote and simply do a signed 64bit compare
				LOpcode qcmp = LOpcode(icmp | LIR64);
				NanoAssert(icmp == LIR_eq && qcmp == LIR_qeq ||
						   icmp == LIR_lt && qcmp == LIR_qlt ||
						   icmp == LIR_le && qcmp == LIR_qle);
				return binaryIns(qcmp, u2p(lhs), i2p(rhs));
			#else
				if (rhs->isconst() && rhs->constval() >= 0)
					return binaryIns(ucmp, lhs, rhs);
			#endif
			}
			else if ((lht == INT_TYPE) && (rht == UINT_TYPE))
			{
				LIns* lhs = localGet(lhsi);
				LIns* rhs = localGet(rhsi);
			#ifdef AVMPLUS_64BIT
				// 32-bit signed and unsigned values fit in 64-bit registers
				// so we can promote and simply do a signed 64bit compare
				LOpcode qcmp = LOpcode(icmp | LIR64);
				NanoAssert(icmp == LIR_eq && qcmp == LIR_qeq ||
						   icmp == LIR_lt && qcmp == LIR_qlt ||
						   icmp == LIR_le && qcmp == LIR_qle);
				return binaryIns(qcmp, i2p(lhs), u2p(rhs));
			#else
				if (lhs->isconst() && lhs->constval() >= 0)
					return binaryIns(ucmp, lhs, rhs);
			#endif
			}

			LIns* lhs = promoteNumberIns(lht, lhsi);
			LIns* rhs = promoteNumberIns(rht, rhsi);
			return binaryIns(fcmp, lhs, rhs);
		}

		return NULL;
	}	

	// set cc's for < operator
	LIns* CodegenLIR::cmpLt(int lhsi, int rhsi)
	{
		LIns *result = cmpOptimization (lhsi, rhsi, LIR_lt, LIR_ult, LIR_flt);
		if (result)
			return result;

		AvmAssert(trueAtom == 13);
		AvmAssert(falseAtom == 5);
		AvmAssert(undefinedAtom == 4);
		LIns* lhs = loadAtomRep(lhsi);
		LIns* rhs = loadAtomRep(rhsi);
		LIns* atom = callIns(FUNCTIONID(compare), 3,
			coreAddr, lhs, rhs);

		// caller will use jt for (a<b) and jf for !(a<b)
		// compare          ^8    <8 
		// true       1101  0101   y 
		// false      0101  1101   n 
		// undefined  0100  1100   n 

		LIns* c = InsConst(8);
		return binaryIns(LIR_lt, binaryIns(LIR_xor, p2i(atom), c), c);
	}

	LIns* CodegenLIR::cmpLe(int lhsi, int rhsi)
	{
		LIns *result = cmpOptimization (lhsi, rhsi, LIR_le, LIR_ule, LIR_fle);
		if (result)
			return result;

		LIns* lhs = loadAtomRep(lhsi);
		LIns* rhs = loadAtomRep(rhsi);
		LIns* atom = callIns(FUNCTIONID(compare), 3,
			coreAddr, rhs, lhs);

		// assume caller will use jt for (a<=b) and jf for !(a<=b)
		// compare          ^1    <=4
		// true       1101  1100  n
		// false      0101  0100  y
		// undefined  0100  0101  n

		LIns* c2 = InsConst(1);
		LIns* c4 = InsConst(4);
		return binaryIns(LIR_le, binaryIns(LIR_xor, p2i(atom), c2), c4);
	}

	LIns* CodegenLIR::cmpEq(const CallInfo *fid, int lhsi, int rhsi)
	{
		LIns *result = cmpOptimization (lhsi, rhsi, LIR_eq, LIR_eq, LIR_feq);
		if (result) {
			return result;
		}

		Traits* lht = state->value(lhsi).traits;
		Traits* rht = state->value(rhsi).traits;
		
		// If we have null and a type that is derived from an Object (but not Object or XML)
		// we can optimize our equal comparison down to a simple ptr comparison. This also
		// works when both types are derived Object types.
		if (((lht == NULL_TYPE) && (rht && !rht->notDerivedObjectOrXML())) || 
			((rht == NULL_TYPE) && (lht && !lht->notDerivedObjectOrXML())) ||
			((rht && !rht->notDerivedObjectOrXML()) && (lht && !lht->notDerivedObjectOrXML())))
		{
			LIns* lhs = localGetp(lhsi);
			LIns* rhs = localGetp(rhsi);
			result = binaryIns(LIR_peq, lhs, rhs);
		}
		else
		{
			LIns* lhs = loadAtomRep(lhsi);
			LIns* rhs = loadAtomRep(rhsi);
			LIns* out = callIns(fid, 3, coreAddr, lhs, rhs);
			result = binaryIns(LIR_peq, out, InsConstAtom(trueAtom));
		}
		return result;
	}

	void CodegenLIR::epilogue(FrameState *state)
	{
		if (outOMem()) return;
		this->state = state;

        if (npe_label.preds) {
            LIns *label = Ins(LIR_label);
			verbose_only( if (frag->lirbuf->names) { frag->lirbuf->names->addName(label, "npe"); })
			mirLabel(npe_label, label);
			callIns(FUNCTIONID(npe), 1, env_param);
		}

        if (interrupt_label.preds) {
            LIns *label = Ins(LIR_label);
			verbose_only( if (frag->lirbuf->names) { frag->lirbuf->names->addName(label, "interrupt"); })
			mirLabel(interrupt_label, label);
			callIns(FUNCTIONID(interrupt), 1, env_param);
		}

        if (info->hasExceptions()) {
            LIns *catchlabel = Ins(LIR_label);
			verbose_only( if (frag->lirbuf->names) { frag->lirbuf->names->addName(catchlabel, "catch");	})
            exBranch->target(catchlabel);

			// exception case
			LIns *exptr = loadIns(LIR_ldp, offsetof(AvmCore, exceptionAddr), coreAddr);
			LIns *exAtom = loadIns(LIR_ldp, offsetof(Exception, atom), exptr);
            localSet(state->verifier->stackBase, exAtom);
			// need to convert exception from atom to native rep, at top of 
			// catch handler.  can't do it here because it could be any type.

			// _ef.beginCatch()
			LIns* pc = loadIns(LIR_ldp, 0, _save_eip);
			LIns* handler = callIns(FUNCTIONID(beginCatch), 5,
				coreAddr, _ef, InsConstPtr(info), pc, exptr);

            int handler_count = info->exceptions->exception_count;
			// jump to catch handler
            LIns *handler_target = loadIns(LIR_ldp, offsetof(ExceptionHandler, target), handler);
			// we dont have LIR_ji yet, so do a compare & branch to each possible target.
			for (int i=0; i < handler_count && !outOMem(); i++)
			{
				ExceptionHandler* h = &info->exceptions->exceptions[i];
                intptr_t handler_pc = h->target;
                if (i+1 < handler_count) {
                    branchIns(LIR_jt, binaryIns(LIR_peq, handler_target, InsConstPtr((void*)handler_pc)), handler_pc);
                } else {
                    branchIns(LIR_j, 0, handler_pc);
                }
			}
        }

        // extend live range of critical stuff
        // fixme -- this should be automatic based on live analysis
        Ins(LIR_live, env_param);
        Ins(LIR_live, vars);
		Ins(LIR_live, coreAddr);
		Ins(LIR_live, undefConst);

        if (info->hasExceptions()) {
            Ins(LIR_live, _ef);
            Ins(LIR_live, _save_eip);
        }

		if (info->setsDxns()) {
			Ins(LIR_live, dxns);
			// dxnsAddrSave might not need to be marked as live -- erring on side of conservatism
			Ins(LIR_live, dxnsAddrSave);
		}

		#ifdef DEBUGGER
		if (core->debugger())
		{
			Ins(LIR_live, csn);
			Ins(LIR_live, varPtrs);
			Ins(LIR_live, varTraits);
		}
		#endif

        for (int i=0, n=patches.size(); i < n; i++) {
            Patch p = patches[i];
            AvmAssert(p.label->bb != 0);
            p.br->target(p.label->bb);
        }

        frag->lastIns = frag->lirbuf->next()-1;
	}

	LIns* CodegenLIR::initMultiname(Multiname* multiname, int& csp, bool isDelete /*=false*/)
	{
		LIns* _tempname = InsAlloc(sizeof(Multiname));

		// copy the flags
		LIns* mFlag = InsConst(multiname->ctFlags());
		storeIns(mFlag, offsetof(Multiname,flags), _tempname);

		LIns* nameAtom = NULL;
		
		if (multiname->isRtname())
		{
			nameAtom = loadAtomRep(csp--);			
		}
		else
		{
			// copy the compile-time name to the temp name
			LIns* mName = InsConstPtr(multiname->name);
			storeIns(mName, offsetof(Multiname,name), _tempname);
		}

		if (multiname->isRtns())
		{
			// intern the runtime namespace and copy to the temp multiname
			LIns* nsAtom = loadAtomRep(csp--);
			LIns* internNs = callIns(FUNCTIONID(internRtns), 2,
				env_param, nsAtom);

			storeIns(internNs, offsetof(Multiname,ns), _tempname);
		}
		else
		{
			// copy the compile-time namespace to the temp multiname
			LIns* mSpace = InsConstPtr(multiname->ns);
			storeIns(mSpace, offsetof(Multiname, ns), _tempname);
		}

		// Call initMultinameLate as the last step, since if a runtime
		// namespace is present, initMultinameLate may clobber it if a
		// QName is provided as index.
		if (nameAtom)
		{
			if (isDelete)
			{
				callIns(FUNCTIONID(initMultinameLateForDelete), 3,
						env_param, _tempname, nameAtom);
			}
			else
			{
				callIns(FUNCTIONID(initMultinameLate), 3,
						coreAddr, _tempname, nameAtom);
			}				
		}

		return _tempname;
	}

	LIns* CodegenLIR::loadToplevel()
	{
		LIns* vtable = loadIns(LIR_ldcp, offsetof(MethodEnv, vtable), env_param);
		return loadIns(LIR_ldcp, offsetof(VTable, toplevel), vtable);
	}

	LIns* CodegenLIR::loadVTable(int i)
	{
		AvmAssert(state->value(i).notNull);
		Traits* t = state->value(i).traits;

		if (t && !t->isMachineType() && t != STRING_TYPE && t != NAMESPACE_TYPE && t != NULL_TYPE)
		{
			// must be a pointer to a scriptobject, and we've done the n
			// all other types are ScriptObject, and we've done the null check
			return loadIns(LIR_ldcp, offsetof(ScriptObject, vtable), localGetp(i));
		}

		LIns* toplevel = loadToplevel();

		int offset;
		if (t == NAMESPACE_TYPE)	offset = offsetof(Toplevel, namespaceClass);
		else if (t == STRING_TYPE)	offset = offsetof(Toplevel, stringClass);
		else if (t == BOOLEAN_TYPE)	offset = offsetof(Toplevel, booleanClass);
		else if (t == NUMBER_TYPE)	offset = offsetof(Toplevel, numberClass);
		else if (t == INT_TYPE)		offset = offsetof(Toplevel, intClass);
		else if (t == UINT_TYPE)	offset = offsetof(Toplevel, uintClass);
		else 
		{
			// *, Object or Void
			LIns* obj = loadAtomRep(i);
			return callIns(FUNCTIONID(toVTable), 2, toplevel, obj);
		}

		// now offset != -1 and we are returning a primitive vtable

		LIns* cc = loadIns(LIR_ldcp, offset, toplevel);
		LIns* cvtable = loadIns(LIR_ldcp, offsetof(ClassClosure, vtable), cc);
		return loadIns(LIR_ldcp, offsetof(VTable, ivtable), cvtable);
	}

	LIns* CodegenLIR::promoteNumberIns(Traits* t, int i)
	{
		if (t == NUMBER_TYPE)
		{
			return localGetq(i);
		}
		if (t == INT_TYPE || t == BOOLEAN_TYPE)
		{
			return i2dIns(localGet(i));
		}
		if (t == UINT_TYPE)
		{
			return u2dIns(localGet(i));
		}
		AvmAssert(false);
		return NULL;
	}

#ifdef AVMPLUS_VERBOSE

	void CodegenLIR::formatOperand(PrintWriter& buffer, LIns* opr)
	{
        if (opr) {
			buffer.format("@%s", frag->lirbuf->names->formatRef(opr));
        }
        else {
			buffer << "0";
        }
	}

#endif /* AVMPLUS_VERBOSE */

	bool CodegenLIR::isCodeContextChanged() const
	{
		return pool->domain->base != NULL;
	}

	/* set position of label */
	void CodegenLIR::mirLabel(CodegenLabel& l, LIns* bb) {
        AvmAssert(bb->isop(LIR_label));
        AvmAssert(l.bb == 0);
        l.bb = bb;
    }

#ifdef DEBUGGER
	void CodegenLIR::emitSampleCheck()
	{
		/* @todo in MIR, inlined sample check doesn't work, help! 
			LIns* takeSample = loadIns(LIR_ld, 0, InsConstPtr(&core->takeSample));
			LIns* br = Ins(MIR_jt, binaryIns(LIR_eq, takeSample, InsConst(0)));
			callIns(FUNCTIONID(sample), 1, coreAddr);
			br->target(lirout->ins0(LIR_label));
		*/
		callIns(FUNCTIONID(sampleCheck), 1, coreAddr);
	}
#endif

#ifdef AVMPLUS_VERBOSE
	bool CodegenLIR::verbose() 
	{
		return state && state->verifier->verbose || pool->verbose;
	}
#endif

    LIns *CodegenLIR::branchIns(LOpcode op, LIns *cond) {
        if (cond) {
            if (!cond->isCmp()) {
                // branching on a non-condition expression, so test (v==0)
                // and invert the sense of the branch.
                cond = lirout->ins_eq0(cond);
                op = LOpcode(op ^ 1);
            }
            if (cond->isconst()) {
                if (op == LIR_jt && cond->constval() || op == LIR_jf && !cond->constval()) {
                    // taken
                    op = LIR_j;
                    cond = 0;
                }
                else {
                    // not taken - no code to emit.
                    // fixme - but what to return? caller wants to patch something.
                    AvmAssert(false);
                }
            }
        }
        return lirout->insBranch(op, cond, 0);
    }

    LIns *CodegenLIR::branchIns(LOpcode op, LIns *cond, uintptr_t pc) {
        LIns *br = branchIns(op, cond);
        patchLater(br, pc);
        return br;
    }

	/* patch the location 'where' with the value of the label */
	void CodegenLIR::patchLater(LIns* br, intptr_t pc)	{
        patchLater(br, state->verifier->getFrameState(pc)->label);
	}

    void CodegenLIR::patchLater(LIns *br, CodegenLabel &l) {
        l.preds++;
        if (l.bb != 0) {
            br->target(l.bb);
        } else {
            patches.add(Patch(br, l));
        }
    }

    LIns* CodegenLIR::InsAlloc(int32_t size) {
        //fixme - why InsAlloc(0)?
        return lirout->insAlloc(size >= 4 ? size : 4);
    }

    PageMgr::PageMgr() : frago(0)
    {}

    PageMgr::~PageMgr() {
        frago->clearFrags();
    }

#ifdef _DEBUG
    class ValidateReader: public LirFilter {
    public:
        ValidateReader(LirFilter *in) : LirFilter(in)
        {}

        LIns* read() {
            LIns *i = in->read();
            if (i) {
                switch (i->opcode()) {
                case LIR_jt:
                case LIR_jf:
                case LIR_j: 
                    AvmAssert(*i->targetAddr() != 0 && *i->targetAddr() == i->oprnd2() && i->oprnd2()->isop(LIR_label));
                    break;
                }
            }
            return i;
        }
    };
#endif

    void CodegenLIR::deadvars_analyze(SortedMap<LIns*, BitSet*, LIST_GCObjects> &labels)
    {
        LirBuffer *lirbuf = frag->lirbuf;
        LIns *catcher = exBranch ? exBranch->getTarget() : 0;
        LIns *vars = lirbuf->sp;
        InsList looplabels(gc);
        BitSet livein(gc, framesize);

        verbose_only(int iter = 0;)
        bool again;
        do {
            again = false;
            livein.reset();
            LirReader in(lirbuf);
            for (LIns *i = in.read(); i != 0; i = in.read()) {
                LOpcode op = i->opcode();
                switch (op) {
                case LIR_ret:
                case LIR_fret:
                    livein.reset();
                    break;
                case LIR_st:
                case LIR_sti:
                case LIR_stq:
                case LIR_stqi:
                    if (i->oprnd2() == vars) {
                        int d = i->immdisp() >> 3;
                        livein.clear(d);
                    }
                    break;
                case LIR_ld:
                case LIR_ldc:
                case LIR_ldq:
                case LIR_ldqc:
                    if (i->oprnd1() == vars) {
                        int d = i->oprnd2()->constval() >> 3;
                        livein.set(gc, d);
                    }
                    break;
                case LIR_label: {
                    // we're at the top of a block, save livein for this block
                    // so it can be propagated to predecessors
                    BitSet *lset = labels.get(i);
                    if (!lset) {
                        lset = new (gc) BitSet(gc, framesize);
                        labels.put(i, lset);
                    }
                    if (lset->setFrom(gc, livein) && !again) {
                        for (int j=0, n=looplabels.size(); j < n; j++) {
                            if (looplabels[j] == i) {
                                again = true;
                                break;
                            }
                        }
                    }
                    break;
                }
                case LIR_j:
                    // the fallthrough path is unreachable, clear it.
                    livein.reset();
                    // fall through to other branch cases
                case LIR_jt:
                case LIR_jf: {
                    // merge the LiveIn sets from each successor:  the fall
                    // through case (livein) and the branch case (lset).
                    LIns *label = i->getTarget();
                    BitSet *lset = labels.get(label);
                    if (lset) {
                        livein.setFrom(gc, *lset);
                    } else {
                        AvmAssert(iter == 0);
                        looplabels.add(label);
                    }
                    break;
                }
                case LIR_qcall:
                case LIR_icall:
                case LIR_fcall:
                    if (catcher && !i->isCse()) {
                        // non-cse call is like a conditional forward branch to the catcher label.
                        // this could be made more precise by checking whether this call
                        // can really throw, and only processing edges to the subset of
                        // reachable catch blocks.  If we haven't seen the catch label yet then
                        // the call is to an exception handling helper (eg beginCatch()) 
                        // that won't throw.
                        BitSet *lset = labels.get(catcher);
                        if (lset)
                            livein.setFrom(gc, *lset);
                    }
                    break;
                }
            }
            verbose_only(iter++;)
        }
        while (again);

        // now make a final pass, modifying LIR to delete dead stores (make them LIR_neartramps)
        verbose_only( if (verbose()) 
            printf("killing dead stores after %d LA iterations.\n",iter);
        )
    }

    void CodegenLIR::deadvars_kill(SortedMap<LIns*, BitSet*, LIST_GCObjects> &labels)
    {
        verbose_only(LirNameMap *names = frag->lirbuf->names;)
        LIns *catcher = exBranch ? exBranch->getTarget() : 0;
        LirBuffer *lirbuf = frag->lirbuf;
        LIns *vars = lirbuf->sp;
        BitSet livein(gc, framesize);
        LirReader in(lirbuf);
        for (LIns *i = in.read(); i != 0; i = in.read()) {
            LOpcode op = i->opcode();
            switch (op) {
                case LIR_ret:
                case LIR_fret:
                    livein.reset();
                    break;
                case LIR_st:
                case LIR_sti:
                case LIR_stq:
                case LIR_stqi:
                    if (i->oprnd2() == vars) {
                        int d = i->immdisp() >> 3;
                        if (!livein.get(d)) {
                            verbose_only(if (names)
                                printf("- %s\n", names->formatIns(i));)
                            i->initOpcode(LIR_neartramp);
                            continue;
                        } else {
                            livein.clear(d);
                        }
                    }
                    break;
                case LIR_ld:
                case LIR_ldc:
                case LIR_ldq:
                case LIR_ldqc:
                    if (i->oprnd1() == vars) {
                        int d = i->oprnd2()->constval() >> 3;
                        livein.set(gc, d);
                    }
                    break;
                case LIR_label: {
                    // we're at the top of a block, save livein for this block
                    // so it can be propagated to predecessors
                    BitSet *lset = labels.get(i);
                    AvmAssert(lset != 0); // all labels have been seen by deadvars_analyze()
                    lset->setFrom(gc, livein);
                    break;
                }
                case LIR_j:
                    // the fallthrough path is unreachable, clear it.
                    livein.reset();
                    // fall through to other branch cases
                case LIR_jt:
                case LIR_jf: {
                    // merge the LiveIn sets from each successor:  the fall
                    // through case (live) and the branch case (lset).
                    BitSet *lset = labels.get(i->getTarget());
                    AvmAssert(lset != 0); // all labels have been seen by deadvars_analyze()
                    // the target LiveIn set (lset) is non-empty,
                    // union it with fall-through set (live).
                    livein.setFrom(gc, *lset);
                    break;
                }
                case LIR_qcall:
                case LIR_icall:
                case LIR_fcall:
                    if (catcher && !i->isCse()) {
                        // non-cse call is like a conditional branch to the catcher label.
                        // this could be made more precise by checking whether this call
                        // can really throw, and only processing edges to the subset of
                        // reachable catch blocks.
                        BitSet *lset = labels.get(catcher);
                        AvmAssert(lset != 0); // this is a forward branch, we have seen the label.
                        // the target LiveIn set (lset) is non-empty,
                        // union it with fall-through set (live).
                        livein.setFrom(gc, *lset);
                    }
                    break;
            }
            verbose_only(if (names) {
                printf("  %s\n", names->formatIns(i));
            })
        }
    }

    /*
     * this is iterative live variable analysis.  We walk backwards through
     * the code.  when we see a load, we mark the variable live, and when
     * we see a store, we mark it dead.  Dead stores are dropped, not returned
     * by read().  
     *
     * at labels, we save the liveIn set associated with that label.
     *
     * at branches, we merge the liveIn sets from the fall through case (which
     * is the current set) and the branch case (which was saved with the label).
     * this filter can be run multiple times, which is required to pick up 
     * loop-carried live variables.
     *
     * once the live sets are stable, the DeadVars.kill flag is set to cause the filter
     * to not only drop dead stores, but overwrite them as tramps so they'll be
     * ignored by any later passes even without using this filter.
     */

    void CodegenLIR::deadvars()
    {
        SortedMap<LIns*, BitSet*, LIST_GCObjects> labels(gc);
        deadvars_analyze(labels);
        deadvars_kill(labels);
        for (int i=0, n = labels.size(); i < n; i++) {
            BitSet *b = labels.at(i);
            delete b;
        }
    }

#ifdef AVMPLUS_JITMAX
    int jitcount = 0;
    int jitmax = 0x7fffffff;
#endif

    void CodegenLIR::emitMD() 
    {
        PERFM_NTPROF("compile");
        debug_only(
            LirReader reader(frag->lirbuf);
            ValidateReader validator(&reader);
            while (validator.read())
            {}
        )

		verbose_only( if (core->config.bbgraph) { 
			StUTF8String cname(info->format(core));
			frag->cfg->fin();
			frag->cfg->print((char*)cname.c_str());
		});
		
        deadvars();

        verbose_only(if (verbose()) {
            live(gc, frag->lirbuf);
        })

        Fragmento *frago = pool->codePages->frago;
        Assembler *assm = frago->assm();
        #ifdef VTUNE
        assm->cgen = this;
        #endif

		verbose_only( StringList asmOutput(gc); )
		verbose_only( assm->_outputCache = &asmOutput; )
        RegAllocMap regMap(gc);
        NInsList loopJumps(gc);
        assm->hasLoop = false;
        assm->beginAssembly(frag, &regMap);
        assm->assemble(frag, loopJumps);
        assm->endAssembly(frag, loopJumps);
        PERFM_TPROF_END();
		
		verbose_only(
            assm->_outputCache = 0;
            for (int i=asmOutput.size()-1; i>=0; --i) {
                assm->outputf("%s",asmOutput.get(i)); 
            }
        );

        PERFM_NVPROF("IR-bytes", frag->lirbuf->byteCount());
		PERFM_NVPROF("IR", frag->lirbuf->insCount());		
		
        LirBuffer *lirbuf = frag->lirbuf;
        frag->releaseLirBuffer();
        for (LirWriter *w = lirout, *wnext; w != 0; w = wnext) {
            wnext = w->out;
            delete w;
        }
        delete lirbuf;

        bool keep = //!info->hasExceptions() && 
            !assm->error();
    #ifdef AVMPLUS_JITMAX
        jitcount++;
        keep = keep && jitcount <= jitmax;
    #endif

        //_nvprof("keep",keep);
        if (keep) {
            // save pointer to generated code
            union {
                Atom (*fp)(MethodEnv*, int, uint32_t*);
                void *vp;
            } u;
            u.vp = frag->code();
            info->impl32 = u.fp;
            // mark method as been JIT'd
            info->flags |= AbstractFunction::JIT_IMPL;
            #if defined AVMPLUS_JITMAX && defined AVMPLUS_VERBOSE
            if (verbose())
                printf("keeping %d, loop=%d\n", jitcount, assm->hasLoop);
            #endif
        } else {
            // assm puked, or we did something untested, so interpret.
            // fixme: need to remove this frag from Fragmento and free everything.
            frag->releaseCode(frago);
            overflow = true;
            #if defined AVMPLUS_JITMAX && defined AVMPLUS_VERBOSE
            if (verbose())
                printf("reverting to interpreter %d assm->error %d \n", jitcount, assm->error());
            #endif
            PERFM_NVPROF("lir-error",1);
        }

       #ifdef VTUNE
       if (keep) 
       {
           AvmAssert(!jitPendingRecords.size());  // all should be resolved by now
           int32_t count = jitInfoList.size(); 
           uint32_t id = 0;        
           for(int i=0; i<count; i++) 
           {
               JITCodeInfo* jitInfo = jitInfoList.get(i);
               if (jitInfo->lineNumTable.size()) {
                   jitInfo->sid = id++;
                   VTune_RegisterMethod(core, jitInfo);
               }
               jitInfo->clear();
           }
           jitInfoList.clear();
       }
       #endif /* VTUNE */
    }

#ifdef VTUNE
   JITCodeInfo* CodegenLIR::jitCurrentInfo()
   {
       if (jitInfoList.size()<1)
           jitPushInfo();
       return jitInfoList[jitInfoList.size()-1];
   }

   void CodegenLIR::jitPushInfo()
   {
       JITCodeInfo* inf = new (gc) JITCodeInfo(gc);
       jitInfoList.add(inf);
       inf->method = info;
   }

   LineNumberRecord* CodegenLIR::jitAddRecord(uint32_t pos, uint32_t filename, uint32_t line, bool pending)
   {
       // create a new record in the table; possibly overwriting an existing one.
       // for pending (i.e. reverse code gen) we want to keep the first line entry we, see
       // for forwards generation we'll take the last one then we encounter
       AvmAssert(pending || (filename && line));
       if (pending && jitCurrentInfo()->lineNumTable.containsKey(pos)) return 0;
       LineNumberRecord* rec = new (gc) LineNumberRecord((Stringp)filename, line);
       jitCurrentInfo()->lineNumTable.put(pos,rec);
       if (pending) jitPendingRecords.add(rec);
       return rec;
   }

   void CodegenLIR::jitFilenameUpdate(uint32_t filename) 
   {
       AvmAssert(filename);
       int32_t size = jitPendingRecords.size();
       for(int32_t i=size-1; i>=0; i--)    {
           LineNumberRecord* rec = jitPendingRecords.get(i);
           if (!rec->filename) {
               rec->filename = (Stringp)filename;
               if (rec->lineno && rec->filename) jitPendingRecords.removeAt(i); // has both fields set?
           }
       }
   }

   void CodegenLIR::jitLineNumUpdate(uint32_t num) 
   {
       AvmAssert(num);
       int32_t size = jitPendingRecords.size();
       for(int32_t i=size-1; i>=0; i--)    {
           LineNumberRecord* rec = jitPendingRecords.get(i);
           if (!rec->lineno)   {
               rec->lineno = num;
               if (rec->lineno && rec->filename) jitPendingRecords.removeAt(i); // has both fields set?
           }
       }
   }

   void CodegenLIR::jitCodePosUpdate(uint32_t pos) 
   {
       JITCodeInfo* inf = jitCurrentInfo();
       if (!inf->endAddr) inf->endAddr = pos;
       inf->startAddr = pos;
   }

#endif

	CodegenIMT::CodegenIMT(PoolObject *pool)
		: pool(pool), overflow(false)
	{}

	CodegenIMT::~CodegenIMT()
	{}

	void CodegenIMT::clearMIRBuffers()
	{}


	void* CodegenIMT::emitImtThunk(ImtBuilder::ImtEntry *e)
	{
        count_imt();

        initCodePages(pool);
        Fragmento *frago = pool->codePages->frago;
		AvmCore *core = pool->core;
		GC *gc = core->gc;

        Fragment *frag = frago->getAnchor(e->virt);
        gc->Free(frag->mergeCounts);
        frag->mergeCounts = 0;
        LirBuffer *lirbuf = frag->lirbuf = new (gc) LirBuffer(frago);
        lirbuf->abi = ABI_FASTCALL;
        LirWriter *lirout = new (gc) LirBufWriter(lirbuf);
        debug_only(
            lirout = new (gc) ValidateWriter(lirout);
        )
        verbose_only(if (pool->verbose) {
            lirout = pushVerboseWriter(gc, lirout, lirbuf);
        })
#ifdef NJ_SOFTFLOAT
        lirout = new (gc) SoftFloatFilter(lirout);
#endif

		// x86-specific notes:
		// the thunk we're generating is really a CDECL function.  We mark
		// it as fastcall to enable the incoming iid arg in EDX.  Assembler
		// doesn't actually know how to pop stack args, so this will end
		// up doing the right thing.  this is very fragile and should be fixed!

		emitStart(lirbuf, lirout, overflow);
		if (overflow)
			return false;

	#ifdef AVMPLUS_IA32
		LIns *iid_param = lirout->insParam(1, 0); // edx
		//env_param = lirout->insParam(2, 0); // stack
		argc_param = lirout->insParam(3, 0); // stack
		ap_param = lirout->insParam(4, 0); // stack
	#else
		// works for abi's that have at least 4 parameter registers
		//env_param = lirout->insParam(0, 0);
		argc_param = lirout->insParam(1, 0);
		ap_param = lirout->insParam(2, 0);
		LIns *iid_param = lirout->insParam(3, 0);
	#endif

	#ifdef NANOJIT_64BIT
		argc_param = lirout->ins1(LIR_qlo, argc_param);
	#endif
	
		LIns *obj = lirout->insLoad(LIR_ldp, ap_param, 0);
		LIns *vtable = lirout->insLoad(LIR_ldcp, obj, offsetof(ScriptObject,vtable));

        verbose_only( if (lirbuf->names) {
            lirbuf->names->addName(argc_param, "argc");
            lirbuf->names->addName(ap_param, "ap");
            lirbuf->names->addName(obj, "this");
            lirbuf->names->addName(vtable, "vtable");
            lirbuf->names->addName(iid_param, "iid");
        })

		AvmAssert(e->next != NULL); // must have 2 or more entries
		while (e->next)
		{
			ImtBuilder::ImtEntry *next = e->next;

			LIns *cmp = lirout->ins2(LIR_peq, iid_param, lirout->insImmPtr((void*)e->virt->iid()));
			LIns *br = lirout->insBranch(LIR_jf, cmp, 0);
			emitCall(lirout, vtable, e);
			br->target(lirout->ins0(LIR_label));

			pool->core->GetGC()->Free(e);
			e = next;
		}

		// last one is unconditional
		emitCall(lirout, vtable, e);
        frag->lastIns = lirbuf->next()-1;

		// now actually generate machine code
        Assembler *assm = frago->assm();

		verbose_only( StringList asmOutput(gc); )
		verbose_only( assm->_outputCache = &asmOutput; )

        RegAllocMap regMap(gc);
        NInsList loopJumps(gc);
        assm->hasLoop = false;
        assm->beginAssembly(frag, &regMap);
        assm->assemble(frag, loopJumps);
        assm->endAssembly(frag, loopJumps);
		verbose_only(
            assm->_outputCache = 0;
            for (int i=asmOutput.size()-1; i>=0; --i) {
                assm->outputf("%s",asmOutput.get(i)); 
            }
        );
        frag->releaseLirBuffer();
		if (assm->error() != None)
		{
			overflow = true;
		}
		return frag->code();
	}

	void CodegenIMT::emitCall(LirWriter *lirout, LIns *vtable, ImtBuilder::ImtEntry *e)
	{
		verbose_only( if (pool->verbose) {
			AvmCore *core = pool->core;
			core->console << "              disp_id="<< e->disp_id << " " << e->virt << " iid " << (uint64_t)e->virt->iid() << "\n";
		})
		// load the concrete env.
		// fixme: this should always be == to the one passed into the thunk, right?
		int32_t offset = (int32_t)(intptr_t)&((VTable*)0)->methods[e->disp_id];
		LIns *env = lirout->insLoad(LIR_ldcp, vtable, offset);
		LIns *target = lirout->insLoad(LIR_ldp, env, offsetof(MethodEnv, impl32));
		LInsp args[] = { ap_param, argc_param, env, target };
		if (Traits::getBuiltinType(e->virt->returnTraits()) == BUILTIN_number) {
			lirout->ins1(LIR_fret, lirout->insCall(FUNCTIONID(fcalli), args));
		} else {
			// this includes the int/uint/bool case, we dont bother truncating and re-widening the result.
			lirout->ins1(LIR_ret, lirout->insCall(FUNCTIONID(acalli), args));
		}
	}
}

namespace nanojit
{
    int StackFilter::getTop(LIns*) {
		AvmAssert(false);
		return 0;
    }

    #ifdef NJ_VERBOSE
    void LirNameMap::formatGuard(LIns*, char*) {
        AvmAssert(false);
    }
    #endif

    void Assembler::asm_bailout(LIns*, Register) {
        AvmAssert(false);
    }

    void Assembler::initGuardRecord(LIns*, GuardRecord*) {
        AvmAssert(false);
    }

	void Fragment::onDestroy() {
		if (root == this) {
			delete mergeCounts;
			delete lirbuf;
			mergeCounts = 0;
			lirbuf = 0;
		}
	}
}

#endif // FEATURE_NANOJIT
