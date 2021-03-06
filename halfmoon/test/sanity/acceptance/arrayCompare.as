/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 2.0/GPL 2.0/LGPL 2.1
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
 * Portions created by the Initial Developer are Copyright (C) 2005-2006
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

function getTestCases() {
    var array1 = new Array();
    var array2 = [];
    var size   = 10;

    // this for loop populates array1 and array2 as follows:
    // array1 = [0,1,2,3,4,....,size - 2,size - 1]
    // array2 = [size - 1, size - 2,...,4,3,2,1,0]
    for (var i = 0; i < size; i++)
    {
        array1.push(i);
        array2.push(size - 1 - i);
    }

	print(String(array1) == String(array2));
    array1.sort();
    array2.sort();
	print(array1);
	print(array2);
	print(String(array1) == String(array2));
}

getTestCases();
