/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WNS/TestFixture.hpp>
#include <WNS/logger/Logger.hpp>

#include <boost/bind.hpp>

namespace wns { namespace events { namespace scheduler { namespace tests { 

     // begin example "wns.events.scheduler.bestpractices.freeFunction.example"
     int globalVariable = 0;

     void freeFunction()
     {
         wns::logger::Logger log("tests", "SchedulerBestPractices");

         MESSAGE_SINGLE(NORMAL, log, "freeFunction is called. Setting globalVariable to 101");

         globalVariable = 101;
     }
     // end example

     // begin example "wns.events.scheduler.bestpractices.classWithCallabck.example"
     class ClassWithCallback
     {
         int memberVariable_;

         wns::logger::Logger logger_;

     public:
         ClassWithCallback() :
             memberVariable_(0),
             logger_("tests", "SchedulerBestPractices")
             {
             }

         void
         callback()
             {
                 MESSAGE_BEGIN(NORMAL, logger_, m, "");
                 m << "ClassWithCallback::callback is called. Setting globalVariable to 101";
                 MESSAGE_END();
                 this->memberVariable_ = 101;
             }

         int
         getMemberVariable()
             {
                 return memberVariable_;
             }

         void
         setMemberVariable(int value)
             {
                 memberVariable_ = value;
             }
     };

     // end example

     class BestPracticesTest :
         public wns::TestFixture
     {
         CPPUNIT_TEST_SUITE( BestPracticesTest );
         CPPUNIT_TEST( bindFreeFunction );
         CPPUNIT_TEST( boostBindFreeFunction );
         CPPUNIT_TEST( boostBindMemberFunctionPtr );
         CPPUNIT_TEST( boostBindMemberFunctionRef );
         CPPUNIT_TEST( boostBindMemberFunctionWithParameter );
         CPPUNIT_TEST_SUITE_END();
     public:
         void prepare();
         void cleanup();
         void bindFreeFunction();
         void boostBindFreeFunction();
         void boostBindMemberFunctionPtr();
         void boostBindMemberFunctionRef();
         void boostBindMemberFunctionWithParameter();
     };

     CPPUNIT_TEST_SUITE_REGISTRATION( BestPracticesTest );

} // tests end
} // scheduler end
} // event end
} // wns end

using namespace wns::events::scheduler::tests;

void
BestPracticesTest::prepare()
{
}

void
BestPracticesTest::cleanup()
{
}

void
BestPracticesTest::bindFreeFunction()
{
    globalVariable = 0;

    // begin example "wns.events.scheduler.bestpractices.bindFreeFunction.example"

    wns::events::scheduler::Interface* scheduler = wns::simulator::getEventScheduler();

    wns::events::scheduler::Callable c = &freeFunction;

    scheduler->scheduleDelay(c, 10.0);

    // end example

    CPPUNIT_ASSERT_EQUAL(0, globalVariable);

    CPPUNIT_ASSERT_EQUAL(0.0, scheduler->getTime());

    scheduler->processOneEvent();

    CPPUNIT_ASSERT_EQUAL(101, globalVariable);

    CPPUNIT_ASSERT_EQUAL(10.0, scheduler->getTime());
}

void
BestPracticesTest::boostBindFreeFunction()
{
    globalVariable = 0;

    // begin example "wns.events.scheduler.bestpractices.boostBindFreeFunction.example"

    wns::events::scheduler::Interface* scheduler = wns::simulator::getEventScheduler();

    wns::events::scheduler::Callable c = boost::bind(&freeFunction);

    scheduler->scheduleNow(c);

    // end example

    CPPUNIT_ASSERT_EQUAL(0, globalVariable);

    CPPUNIT_ASSERT_EQUAL(0.0, scheduler->getTime());

    scheduler->processOneEvent();

    CPPUNIT_ASSERT_EQUAL(101, globalVariable);

    CPPUNIT_ASSERT_EQUAL(0.0, scheduler->getTime());
}

void
BestPracticesTest::boostBindMemberFunctionPtr()
{
    // begin example "wns.events.scheduler.bestpractices.boostBindMemberFunction.example"

    wns::events::scheduler::Interface* scheduler = wns::simulator::getEventScheduler();

    // Create a Smart Pointer to a new ClassWithCallback instance
    ClassWithCallback* classWithCallbackPtr = new ClassWithCallback();

    wns::events::scheduler::Callable c =
        boost::bind(&ClassWithCallback::callback,
                    classWithCallbackPtr);

    scheduler->scheduleDelay(c, 10.0);

    // end example

    CPPUNIT_ASSERT_EQUAL(0.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(0, classWithCallbackPtr->getMemberVariable());

    scheduler->processOneEvent();

    CPPUNIT_ASSERT_EQUAL(10.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(101, classWithCallbackPtr->getMemberVariable());

    delete classWithCallbackPtr;
}

void
BestPracticesTest::boostBindMemberFunctionRef()
{
    // begin example "wns.events.scheduler.bestpractices.boostBindMemberFunctionRef.example"

    wns::events::scheduler::Interface* scheduler = wns::simulator::getEventScheduler();

    ClassWithCallback classWithCallbackInstance;

    wns::events::scheduler::Callable c =
        boost::bind(&ClassWithCallback::callback,
                    boost::ref(classWithCallbackInstance));

    scheduler->scheduleDelay(c, 10.0);

    // end example
    CPPUNIT_ASSERT_EQUAL(0.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(0, classWithCallbackInstance.getMemberVariable());

    scheduler->processOneEvent();

    CPPUNIT_ASSERT_EQUAL(10.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(101, classWithCallbackInstance.getMemberVariable());

}

void
BestPracticesTest::boostBindMemberFunctionWithParameter()
{
    // begin example "wns.events.scheduler.bestpractices.boostBindMemberFunctionParam.example"
    wns::events::scheduler::Interface* scheduler = wns::simulator::getEventScheduler();

    ClassWithCallback* classWithCallbackPtr = new ClassWithCallback();

    wns::events::scheduler::Callable c1 =
        boost::bind(
            &ClassWithCallback::setMemberVariable,
            classWithCallbackPtr,
            201);

    wns::events::scheduler::Callable c2 =
        boost::bind(
            &ClassWithCallback::setMemberVariable,
            classWithCallbackPtr,
            302);

    scheduler->scheduleDelay(c1, 11.0);

    scheduler->scheduleDelay(c2, 10.0);

    // end example

    CPPUNIT_ASSERT_EQUAL(0.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(0, classWithCallbackPtr->getMemberVariable());

    scheduler->processOneEvent();

    CPPUNIT_ASSERT_EQUAL(10.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(302, classWithCallbackPtr->getMemberVariable());

    scheduler->processOneEvent();

    CPPUNIT_ASSERT_EQUAL(11.0, scheduler->getTime());

    CPPUNIT_ASSERT_EQUAL(201, classWithCallbackPtr->getMemberVariable());

    delete classWithCallbackPtr;
}

