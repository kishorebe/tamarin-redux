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
 * Portions created by the Initial Developer are Copyright (C) 2008
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

// Generated by utils/peephole.as

#include "avmplus.h"

namespace avmplus
{
#ifdef AVMPLUS_PEEPHOLE_OPTIMIZER

const WordcodeEmitter::peep_state_t WordcodeEmitter::states[] = {
//n  s  t  g  f
{ 0, 0, 0, 0, 0 }, // Invalid
{ 0, 0, 0, 5, 0 }, //    getlocal getlocal getlocal getlocal getlocal
{ 1, 0, 0, 4, 0 }, //    getlocal getlocal getlocal getlocal
{ 1, 0, 1, 3, 0 }, //    getlocal getlocal getlocal
{ 0, 3, 0, 7, 48 }, //    getlocal getlocal add setlocal
{ 1, 0, 2, 6, 0 }, //    getlocal getlocal add
{ 0, 0, 0, 8, 0 }, //    getlocal getlocal subtract
{ 0, 0, 0, 9, 0 }, //    getlocal getlocal multiply
{ 0, 0, 0, 10, 0 }, //    getlocal getlocal divide
{ 0, 0, 0, 11, 0 }, //    getlocal getlocal modulo
{ 0, 0, 0, 12, 0 }, // 10  getlocal getlocal bitand
{ 0, 0, 0, 13, 0 }, //    getlocal getlocal bitor
{ 0, 0, 0, 14, 0 }, //    getlocal getlocal bitxor
{ 0, 0, 0, 15, 0 }, //    getlocal getlocal iflt
{ 0, 0, 0, 16, 0 }, //    getlocal getlocal ifnlt
{ 0, 0, 0, 17, 0 }, //    getlocal getlocal ifle
{ 0, 0, 0, 18, 0 }, //    getlocal getlocal ifnle
{ 0, 0, 0, 19, 0 }, //    getlocal getlocal ifgt
{ 0, 0, 0, 20, 0 }, //    getlocal getlocal ifngt
{ 0, 0, 0, 21, 0 }, //    getlocal getlocal ifge
{ 0, 0, 0, 22, 0 }, // 20  getlocal getlocal ifnge
{ 0, 0, 0, 23, 0 }, //    getlocal getlocal ifeq
{ 0, 0, 0, 24, 0 }, //    getlocal getlocal ifne
{ 0, 0, 0, 25, 0 }, //    getlocal getlocal ifstricteq
{ 0, 0, 0, 26, 0 }, //    getlocal getlocal ifstrictne
{ 21, 0, 3, 2, 0 }, //    getlocal getlocal
{ 0, 0, 0, 27, 0 }, //    getlocal pushbits add
{ 0, 0, 0, 28, 0 }, //    getlocal pushbits subtract
{ 0, 0, 0, 29, 0 }, //    getlocal pushbits multiply
{ 0, 0, 0, 30, 0 }, //    getlocal pushbits divide
{ 0, 0, 0, 31, 0 }, // 30  getlocal pushbits bitand
{ 0, 0, 0, 32, 0 }, //    getlocal pushbits bitor
{ 0, 0, 0, 33, 0 }, //    getlocal pushbits bitxor
{ 0, 0, 0, 34, 0 }, //    getlocal pushbits iflt
{ 0, 0, 0, 35, 0 }, //    getlocal pushbits ifnlt
{ 0, 0, 0, 36, 0 }, //    getlocal pushbits ifle
{ 0, 0, 0, 37, 0 }, //    getlocal pushbits ifnle
{ 0, 0, 0, 38, 0 }, //    getlocal pushbits ifgt
{ 0, 0, 0, 39, 0 }, //    getlocal pushbits ifngt
{ 0, 0, 0, 40, 0 }, //    getlocal pushbits ifge
{ 0, 0, 0, 41, 0 }, // 40  getlocal pushbits ifnge
{ 0, 0, 0, 42, 0 }, //    getlocal pushbits ifeq
{ 0, 0, 0, 43, 0 }, //    getlocal pushbits ifne
{ 0, 0, 0, 44, 0 }, //    getlocal pushbits ifstricteq
{ 0, 0, 0, 45, 0 }, //    getlocal pushbits ifstrictne
{ 19, 0, 24, 0, 0 }, //    getlocal pushbits
{ 2, 0, 43, 1, 0 }, //    getlocal
{ 0, 1, 0, 47, 46 }, //    setlocal getlocal
{ 1, 0, 45, 46, 0 }, //    setlocal
{ 0, 0, 0, 48, 0 }, //    swap pop
{ 1, 0, 46, 0, 0 }, // 50  swap
};

const WordcodeEmitter::peep_transition_t WordcodeEmitter::transitions[] = {
{ WOP_getlocal, 1 },
{ WOP_getlocal, 2 },
{ WOP_setlocal, 4 },
{ WOP_ifnlt, 14 },
{ WOP_ifnle, 16 },
{ WOP_ifngt, 18 },
{ WOP_ifnge, 20 },
{ WOP_ifeq, 21 },
{ WOP_ifne, 22 },
{ WOP_iflt, 13 },
{ WOP_ifle, 15 }, // 10
{ WOP_ifgt, 17 },
{ WOP_ifge, 19 },
{ WOP_ifstricteq, 23 },
{ WOP_ifstrictne, 24 },
{ WOP_getlocal, 3 },
{ WOP_add, 5 },
{ WOP_subtract, 6 },
{ WOP_multiply, 7 },
{ WOP_divide, 8 },
{ WOP_modulo, 9 }, // 20
{ WOP_bitand, 10 },
{ WOP_bitor, 11 },
{ WOP_bitxor, 12 },
{ WOP_ifnlt, 34 },
{ WOP_ifnle, 36 },
{ WOP_ifngt, 38 },
{ WOP_ifnge, 40 },
{ WOP_ifeq, 41 },
{ WOP_ifne, 42 },
{ WOP_iflt, 33 }, // 30
{ WOP_ifle, 35 },
{ WOP_ifgt, 37 },
{ WOP_ifge, 39 },
{ WOP_ifstricteq, 43 },
{ WOP_ifstrictne, 44 },
{ WOP_add, 26 },
{ WOP_subtract, 27 },
{ WOP_multiply, 28 },
{ WOP_divide, 29 },
{ WOP_bitand, 30 }, // 40
{ WOP_bitor, 31 },
{ WOP_bitxor, 32 },
{ WOP_getlocal, 25 },
{ WOP_pushbits, 45 },
{ WOP_getlocal, 47 },
{ WOP_pop, 49 },
};

const uint16 WordcodeEmitter::toplevel[] = {
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    nop                  throw                getsuper             setsuper             dxns                 dxnslate            
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    ifnlt                ifnle                ifngt                ifnge               
0, 0, 0, 0, 0, 0, 0, 0, // jump                 iftrue               iffalse              ifeq                 ifne                 iflt                 ifle                 ifgt                
0, 0, 0, 0, 0, 0, 0, 0, // ifge                 ifstricteq           ifstrictne           lookupswitch         pushwith             popscope             nextname             hasnext             
0, 0, 0, 0, 0, 0, 0, 0, // pushnull             pushundefined        -                    nextvalue            -                    -                    pushtrue             pushfalse           
0, 0, 0, 50, 0, 0, 0, 0, // pushnan              pop                  dup                  swap                 pushstring           -                    -                    pushdouble          
0, 0, 0, 0, 0, 0, 0, 0, // pushscope            pushnamespace        hasnext2             -                    -                    li8                  li16                 li32                
0, 0, 0, 0, 0, 0, 0, 0, // lf32                 lf64                 si8                  si16                 si32                 sf32                 sf64                 -                   
0, 0, 0, 0, 0, 0, 0, 0, // newfunction          call                 construct            callmethod           callstatic           callsuper            callproperty         returnvoid          
0, 0, 0, 0, 0, 0, 0, 0, // returnvalue          constructsuper       constructprop        -                    callproplex          -                    callsupervoid        callpropvoid        
0, 0, 0, 0, 0, 0, 0, 0, // sxi1                 sxi8                 sxi16                applytype            -                    newobject            newarray             newactivation       
0, 0, 0, 0, 0, 0, 0, 0, // newclass             getdescendants       newcatch             -                    -                    findpropstrict       findproperty         finddef             
0, 0, 46, 48, 0, 0, 0, 0, // getlex               setproperty          getlocal             setlocal             getglobalscope       getscopeobject       getproperty          getouterscope       
0, 0, 0, 0, 0, 0, 0, 0, // initproperty         -                    deleteproperty       -                    getslot              setslot              getglobalslot        setglobalslot       
0, 0, 0, 0, 0, 0, 0, 0, // convert_s            esc_xelem            esc_xattr            convert_i            convert_u            convert_d            convert_b            convert_o           
0, 0, 0, 0, 0, 0, 0, 0, // checkfilter          -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // coerce               coerce_b             coerce_a             coerce_i             coerce_d             coerce_s             astype               astypelate          
0, 0, 0, 0, 0, 0, 0, 0, // coerce_u             coerce_o             -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // negate               increment            inclocal             decrement            declocal             typeof               not                  bitnot              
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // add                  subtract             multiply             divide               modulo               lshift               rshift               urshift             
0, 0, 0, 0, 0, 0, 0, 0, // bitand               bitor                bitxor               equals               strictequals         lessthan             lessequals           greaterthan         
0, 0, 0, 0, 0, 0, 0, 0, // greaterequals        instanceof           istype               istypelate           in                   -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // increment_i          decrement_i          inclocal_i           declocal_i           negate_i             add_i                subtract_i           multiply_i          
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // getlocal0            getlocal1            getlocal2            getlocal3            setlocal0            setlocal1            setlocal2            setlocal3           
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    debug               
0, 0, 0, 0, 0, 0, 0, 0, // debugline            debugfile            -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    pushbits             push_doublebits      get2locals           get3locals           get4locals           get5locals           storelocal          
0, 0, 0, 0, 0, 0, 0, 0, // add_ll               add_set_lll          subtract_ll          multiply_ll          divide_ll            modulo_ll            bitand_ll            bitor_ll            
0, 0, 0, 0, 0, 0, 0, 0, // bitxor_ll            add_lb               subtract_lb          multiply_lb          divide_lb            bitand_lb            bitor_lb             bitxor_lb           
0, 0, 0, 0, 0, 0, 0, 0, // iflt_ll              ifnlt_ll             ifle_ll              ifnle_ll             ifgt_ll              ifngt_ll             ifge_ll              ifnge_ll            
0, 0, 0, 0, 0, 0, 0, 0, // ifeq_ll              ifne_ll              ifstricteq_ll        ifstrictne_ll        iflt_lb              ifnlt_lb             ifle_lb              ifnle_lb            
0, 0, 0, 0, 0, 0, 0, 0, // ifgt_lb              ifngt_lb             ifge_lb              ifnge_lb             ifeq_lb              ifne_lb              ifstricteq_lb        ifstrictne_lb       
0, 0, 0, 0, 0, 0, 0, 0, // swap_pop             findpropglobal       findpropglobalstrict -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                    -                    -                   
0, 0, 0, 0, 0, 0, // -                    -                    -                    -                    -                    -                   
};

bool WordcodeEmitter::commit(uint32 action)
{
    switch (action) {
    case 1:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal));
        if (I[0][1] < 4) {
            S[0] = WOP_getlocal0 + I[0][1];
            R[0] = NEW_OPCODE(WOP_getlocal0 + I[0][1]);
            return replace(1,1);
        }
        return false;
    case 2:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_get2locals;
            R[0] = NEW_OPCODE(WOP_get2locals);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(2,2);
        }
        return false;
    case 3:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_getlocal));
        if (I[0][1] < 1024 && I[1][1] < 1024 && I[2][1] < 1024) {
            S[0] = WOP_get3locals;
            R[0] = NEW_OPCODE(WOP_get3locals);
            R[1] = (I[2][1] << 20) | (I[1][1] << 10) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 4:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_getlocal) && I[3][0] == NEW_OPCODE(WOP_getlocal));
        if (I[0][1] < 256 && I[1][1] < 256 && I[2][1] < 256 && I[3][1] < 256) {
            S[0] = WOP_get4locals;
            R[0] = NEW_OPCODE(WOP_get4locals);
            R[1] = (I[3][1] << 24) | (I[2][1] << 16) | (I[1][1] << 8) | I[0][1];
            return replace(4,2);
        }
        return false;
    case 5:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_getlocal) && I[3][0] == NEW_OPCODE(WOP_getlocal) && I[4][0] == NEW_OPCODE(WOP_getlocal));
        if (I[0][1] < 64 && I[1][1] < 64 && I[2][1] < 64 && I[3][1] < 64 && I[4][1] < 64) {
            S[0] = WOP_get5locals;
            R[0] = NEW_OPCODE(WOP_get5locals);
            R[1] = (I[4][1] << 24) | (I[3][1] << 18) | (I[2][1] << 12) | (I[1][1] << 6) | I[0][1];
            return replace(5,2);
        }
        return false;
    case 6:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_add));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_add_ll;
            R[0] = NEW_OPCODE(WOP_add_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 7:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_add) && I[3][0] == NEW_OPCODE(WOP_setlocal));
        if (I[0][1] < 1024 && I[1][1] < 1024 && I[3][1] < 1024) {
            S[0] = WOP_add_set_lll;
            R[0] = NEW_OPCODE(WOP_add_set_lll);
            R[1] = (I[3][1] << 20) | (I[1][1] << 10) | I[0][1];
            return replace(4,2);
        }
        return false;
    case 8:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_subtract));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_subtract_ll;
            R[0] = NEW_OPCODE(WOP_subtract_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 9:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_multiply));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_multiply_ll;
            R[0] = NEW_OPCODE(WOP_multiply_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 10:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_divide));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_divide_ll;
            R[0] = NEW_OPCODE(WOP_divide_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 11:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_modulo));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_modulo_ll;
            R[0] = NEW_OPCODE(WOP_modulo_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 12:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_bitand));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_bitand_ll;
            R[0] = NEW_OPCODE(WOP_bitand_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 13:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_bitor));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_bitor_ll;
            R[0] = NEW_OPCODE(WOP_bitor_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 14:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_bitxor));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            S[0] = WOP_bitxor_ll;
            R[0] = NEW_OPCODE(WOP_bitxor_ll);
            R[1] = (I[1][1] << 16) | I[0][1];
            return replace(3,2);
        }
        return false;
    case 15:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_iflt));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_iflt_ll;
            R[0] = NEW_OPCODE(WOP_iflt_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 16:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifnlt));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifnlt_ll;
            R[0] = NEW_OPCODE(WOP_ifnlt_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 17:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifle));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifle_ll;
            R[0] = NEW_OPCODE(WOP_ifle_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 18:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifnle));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifnle_ll;
            R[0] = NEW_OPCODE(WOP_ifnle_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 19:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifgt));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifgt_ll;
            R[0] = NEW_OPCODE(WOP_ifgt_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 20:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifngt));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifngt_ll;
            R[0] = NEW_OPCODE(WOP_ifngt_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 21:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifge));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifge_ll;
            R[0] = NEW_OPCODE(WOP_ifge_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 22:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifnge));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifnge_ll;
            R[0] = NEW_OPCODE(WOP_ifnge_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 23:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifeq));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifeq_ll;
            R[0] = NEW_OPCODE(WOP_ifeq_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 24:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifne));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifne_ll;
            R[0] = NEW_OPCODE(WOP_ifne_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 25:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifstricteq));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifstricteq_ll;
            R[0] = NEW_OPCODE(WOP_ifstricteq_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 26:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal) && I[2][0] == NEW_OPCODE(WOP_ifstrictne));
        if (I[0][1] < 65536 && I[1][1] < 65536) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifstrictne_ll;
            R[0] = NEW_OPCODE(WOP_ifstrictne_ll);
            R[1] = I[2][1];
            R[2] = (I[1][1] << 16) | I[0][1];
            return replace(3,3,true);
        }
        return false;
    case 27:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_add));
        if (true) {
            S[0] = WOP_add_lb;
            R[0] = NEW_OPCODE(WOP_add_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 28:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_subtract));
        if (true) {
            S[0] = WOP_subtract_lb;
            R[0] = NEW_OPCODE(WOP_subtract_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 29:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_multiply));
        if (true) {
            S[0] = WOP_multiply_lb;
            R[0] = NEW_OPCODE(WOP_multiply_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 30:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_divide));
        if (true) {
            S[0] = WOP_divide_lb;
            R[0] = NEW_OPCODE(WOP_divide_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 31:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_bitand));
        if (true) {
            S[0] = WOP_bitand_lb;
            R[0] = NEW_OPCODE(WOP_bitand_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 32:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_bitor));
        if (true) {
            S[0] = WOP_bitor_lb;
            R[0] = NEW_OPCODE(WOP_bitor_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 33:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_bitxor));
        if (true) {
            S[0] = WOP_bitxor_lb;
            R[0] = NEW_OPCODE(WOP_bitxor_lb);
            R[1] = I[0][1];
            R[2] = I[1][1];
            return replace(3,3);
        }
        return false;
    case 34:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_iflt));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_iflt_lb;
            R[0] = NEW_OPCODE(WOP_iflt_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 35:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifnlt));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifnlt_lb;
            R[0] = NEW_OPCODE(WOP_ifnlt_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 36:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifle));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifle_lb;
            R[0] = NEW_OPCODE(WOP_ifle_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 37:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifnle));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifnle_lb;
            R[0] = NEW_OPCODE(WOP_ifnle_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 38:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifgt));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifgt_lb;
            R[0] = NEW_OPCODE(WOP_ifgt_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 39:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifngt));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifngt_lb;
            R[0] = NEW_OPCODE(WOP_ifngt_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 40:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifge));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifge_lb;
            R[0] = NEW_OPCODE(WOP_ifge_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 41:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifnge));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifnge_lb;
            R[0] = NEW_OPCODE(WOP_ifnge_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 42:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifeq));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifeq_lb;
            R[0] = NEW_OPCODE(WOP_ifeq_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 43:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifne));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifne_lb;
            R[0] = NEW_OPCODE(WOP_ifne_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 44:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifstricteq));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifstricteq_lb;
            R[0] = NEW_OPCODE(WOP_ifstricteq_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 45:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_getlocal) && I[1][0] == NEW_OPCODE(WOP_pushbits) && I[2][0] == NEW_OPCODE(WOP_ifstrictne));
        if (true) {
            undoRelativeOffsetInJump();
            S[0] = WOP_ifstrictne_lb;
            R[0] = NEW_OPCODE(WOP_ifstrictne_lb);
            R[1] = I[2][1];
            R[2] = I[0][1];
            R[3] = I[1][1];
            return replace(3,4,true);
        }
        return false;
    case 46:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_setlocal));
        if (I[0][1] < 4) {
            S[0] = WOP_setlocal0 + I[0][1];
            R[0] = NEW_OPCODE(WOP_setlocal0 + I[0][1]);
            return replace(1,1);
        }
        return false;
    case 47:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_setlocal) && I[1][0] == NEW_OPCODE(WOP_getlocal));
        if (I[0][1] == I[1][1]) {
            S[0] = WOP_storelocal;
            R[0] = NEW_OPCODE(WOP_storelocal);
            R[1] = I[0][1];
            return replace(2,2);
        }
        return false;
    case 48:
        AvmAssert(I[0][0] == NEW_OPCODE(WOP_swap) && I[1][0] == NEW_OPCODE(WOP_pop));
        if (true) {
            S[0] = WOP_swap_pop;
            R[0] = NEW_OPCODE(WOP_swap_pop);
            return replace(2,1);
        }
        return false;
    default:
        AvmAssert(!"Should not happen");
        return false;
    }
}

#endif // AVMPLUS_PEEPHOLE_OPTIMIZER
}
