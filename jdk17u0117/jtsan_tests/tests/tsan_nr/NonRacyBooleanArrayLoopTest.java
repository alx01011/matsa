/*
 * Copyright (c) 2019 Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2019 Google and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/* @test NonRacyBooleanArrayLoopTest
 * @summary Test a simple Java non-racy memory access via a boolean array.
 * @library /test/lib
 * @build AbstractLoop TsanRunner
 * @run main NonRacyBooleanArrayLoopTest
 */

 import java.io.IOException;
 
 
class NonRacyBooleanArrayLoopTest extends Object {
   private boolean[] x = new boolean[2];
 
   protected synchronized void run(int i) {
     x[0] = !x[0];
   }
 
   public static void main(String[] args) throws InterruptedException {
    NonRacyBooleanArrayLoopTest test = new NonRacyBooleanArrayLoopTest();

     final Thread t1 =
     new Thread(
         () -> {
           for (int i = 0; i < 50000; i++) {
                test.run(i);
           }
         });
    final Thread t2 =
     new Thread(
         () -> {
           for (int i = 0; i < 50000; i++) {
             test.run(i);
           }
         });

        t1.start();
        t2.start();
     
        t1.join();
        t2.join();

   }
 }