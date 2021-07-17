/** @file Shop.cpp
 * @author Korosh Moosavi
 * @date 2021-05-10
 * 
 * Modified version of shop_org.cpp provided by Prof. Dimpsey
 *
 * Shop.cpp file:
 * The Shop class is a monitor for barber and customer threads
 * Shop coordinates the interactions between these threads
 *   using various signals, condition variables, and a mutex
 *
 * Assumptions:
 * The driver using this class calls the below methods in an appropriate
 *   order (i.e. helloCustomer() -> byeCustomer() )
 * Driver validates parameters before passing them into these methods
 */

#include "Shop.h"

// --------------------------- Parameter constructor
// Uses init() to initialize all array values and mutex/conditions
// Sets max barbers and chairs to default values if parameter is invalid
// 
// pre: None
// param: num_barbers  Maximum allowed number of barbers working at a time
// param: num_chairs   Maximum number of waiting customers
// post: Shop object can be passed threads of customers and barbers
//
Shop::Shop(int num_barbers, int num_chairs) :                  // Use default values if parameters are invalid
   max_waiting_cust_((num_chairs >= 0) ? num_chairs : kDefaultNumChairs), 
   max_working_barb_((num_barbers > 0) ? num_barbers : kDefaultBarbers),
   cust_drops_(0),
   waiting_customers_(0),
   sleeping_barbs_(0)
{
   customer_in_chair_ = new int[max_working_barb_];
   in_service_ = new bool[max_working_barb_];
   money_paid_ = new bool[max_working_barb_];
   cond_customer_served_ = new pthread_cond_t[max_working_barb_];
   cond_barber_paid_ = new pthread_cond_t[max_working_barb_];
   cond_barber_sleeping_ = new pthread_cond_t[max_working_barb_];

   init();                                                     // Initialize arrays and conditions/mutex
};

// --------------------------- Default constructor
// Uses init() to initialize all array values and mutex/conditions
// Sets max barbers and chairs to default values
// 
// pre: None
// post: Shop object can be passed threads of customers and barbers
//
Shop::Shop() :
   max_waiting_cust_(kDefaultNumChairs),                       // Use default values
   max_working_barb_(kDefaultBarbers),
   cust_drops_(0),
   waiting_customers_(0),
   sleeping_barbs_(0)
{
   customer_in_chair_ = new int[max_working_barb_];
   in_service_ = new bool[max_working_barb_];
   money_paid_ = new bool[max_working_barb_];
   cond_customer_served_ = new pthread_cond_t[max_working_barb_];
   cond_barber_paid_ = new pthread_cond_t[max_working_barb_];
   cond_barber_sleeping_ = new pthread_cond_t[max_working_barb_];

   init();                                                     // Initialize arrays and conditions/mutex
};

// --------------------------- Destructor
// Deletes the dynamically allocated arrays in this class (6 total)
//
Shop::~Shop()
{
   delete []customer_in_chair_;
   delete []in_service_;
   delete []money_paid_;
   delete []cond_customer_served_;
   delete []cond_barber_paid_;
   delete []cond_barber_sleeping_;
}

// --------------------------- void init()
// Initializes every element of every array used in this class
//   as well as the mutex and condition variables
// 
// pre: None
// post: This Shop object is initialized and ready for operation
//
void Shop::init()
{
   pthread_mutex_init(&mutex_, NULL);
   pthread_cond_init(&cond_customers_waiting_, NULL);

   for (int i = 0; i < max_working_barb_; i++) {
      customer_in_chair_[i] = 0;
      in_service_[i] = false;
      money_paid_[i] = false;

      pthread_cond_init(&cond_customer_served_[i], NULL);
      pthread_cond_init(&cond_barber_paid_[i], NULL);
      pthread_cond_init(&cond_barber_sleeping_[i], NULL);
   }
}

// --------------------------- string int2string(int)
// Uses a stringstream to convert an int into a string
// 
// pre: None
// param: i  int to be converted
// return: string containing i
//
string Shop::int2string(int i)
{
   stringstream out;
   out << i;
   return out.str();
}

// --------------------------- void print(int, string)
// Prints a preformatted output based on the ID passed in
// If the person is negative their ID is of a barber, otherwise
//   it is of a customer
// Output format is as follows:
//   customer[person]:  message
//      OR 
//   barber [-person]:  message
// 
// pre: None
// param: person   ID of thread printing message
// param: message  Message being output for that thread
// post: Message is output to console as indicated above
//
void Shop::print(int person, string message)
{
   bool isBarb = false;
   if (person <= 0)                                            // Barber IDs are passed as negative values
   {                                                           // Customer IDs begin at +1
      isBarb = true;
      person = -person;
   }
   cout << ((!isBarb) ? "customer[" : "barber  [") << person << "]: " << message << endl;
}

// --------------------------- int visitShop(int)
// First customer method, closely tied with assignBarber(int).
// Uses mutex start to finish with a wait call in the case that there are
//   waiting chairs but no available barber.
// All paths lead to assignBarber(int) which returns an available barber's ID
// This barber is then set as in service and woken if sleeping
// Unlike barbID, custID is not used for any indexing so the runtime value will
//   be the same as the printout value.
//
// pre: Calling method either ensures against or handles the case of not finding an open barber
// param: custID  ID of the customer calling this method
// return: Returns the ID of the first available barber found or -1 if none were found
//
int Shop::visitShop(int custID)
{
   int barbID;
   pthread_mutex_lock(&mutex_);

   if (max_waiting_cust_ == 0)                                 // No waiting chairs, only service chairs
   {
      barbID = assignBarber(custID);                           // Look for an open service chair
      if (barbID == -1)       
      {
         print(custID, "leaves the shop because of no available service chairs.");
         ++cust_drops_;

         pthread_mutex_unlock(&mutex_);                        // -1 returned means no open service chair
         return barbID;                                        //   was found and outputs that the
      }                                                        //   customer leaves the shop
   }    
   else                                                        // There are waiting chairs
   {
      if (max_waiting_cust_ == waiting_customers_)             // If all waiting chairs are full:
      {
         print(custID, "leaves the shop because of no available waiting chairs.");
         ++cust_drops_;

         pthread_mutex_unlock(&mutex_);                        // Leave the shop
         return -1;
      }

      barbID = assignBarber(custID);                           // Look for an open service chair

      if (barbID == -1)                                        // If a chair was not found:
      {
         waiting_customers_++;                                 // Increment waiting customer count
         print(custID, "takes a waiting chair. # waiting seats available = " 
                     + int2string(max_waiting_cust_ - waiting_customers_));
         pthread_cond_wait(&cond_customers_waiting_, &mutex_); // Wait

         barbID = assignBarber(custID);                        // Look for an open service chair
         if (barbID == -1)          
         {
            print(custID, "leaves the shop because of no available service chairs.");
            ++cust_drops_;
            waiting_customers_--;

            pthread_mutex_unlock(&mutex_);
            return barbID;
         }
         waiting_customers_--;                                 // Decrement waiting customer count
      }
   }
   print(custID, "moves to service chair[" 
                  + int2string(barbID + 1)
                  + string("], # waiting seats available = ") 
                  + int2string(max_waiting_cust_ - waiting_customers_));

   in_service_[barbID] = true;

   // wake up the barber just in case if he is sleeping
   pthread_cond_signal(&cond_barber_sleeping_[barbID]);

   pthread_mutex_unlock(&mutex_);
   return barbID;
}

// --------------------------- int assignBarber(int)
// A factored out function from the visitShop(int) method
// Searches through barber chairs and assigns custID to the
//   first empty chair found
// Returns -1 if no chairs are available, returns the chair ID otherwise
// 
// pre: None
// param: custID  ID of customer to assign to an open chair
// post: custID is assigned to a barber chair
// return: -1 or ID of the barber whose chair the customer is in
//
int Shop::assignBarber(int custID)
{
   for (int barbID = 0; barbID < max_working_barb_; barbID++) {// Iterate through barbers 

      if (customer_in_chair_[barbID] == 0) {                   // until one with an empty chair is found
         customer_in_chair_[barbID] = custID;                  // Assign custID to that chair
         sleeping_barbs_--;
         return barbID;                                        // Return ID of barber at the chair
      }
   }

   return -1;
}

// --------------------------- void leaveShop(int, int)
// Second customer method.
// Uses mutex start to finish with a wait call for the barber to finish service
// Customer then pays barber and signals him
//
// pre: barberID is a valid return value from a preceding call to visitShop(int)
// param: custID    ID of the customer calling this method
// param: barberID  ID of the barber serving this customer
// post: Customer thread has set and signaled "paid" for their barber
//
void Shop::leaveShop(int custID, int barberID)
{
   int barbID = barberID;
   pthread_mutex_lock(&mutex_);

   // Wait for service to be completed
   print(custID, "wait for barber[" 
                  + int2string(barbID + 1)
                  + string("] to be done with hair-cut"));
   while (in_service_[barbID] == true)    {
      pthread_cond_wait(&cond_customer_served_[barbID], &mutex_);
   }

   // Pay the barber and signal barber appropriately
   money_paid_[barbID] = true;
   pthread_cond_signal(&cond_barber_paid_[barbID]);
   print(custID, "says good-bye to barber[" 
                  + int2string(barbID + 1) 
                  + string("]"));

   pthread_mutex_unlock(&mutex_);
}

// --------------------------- void helloCustomer(int)
// First barber method.
// Uses mutex start to finish with a wait call for a customer to sit in his chair
// Waits for a customer to come get them, sleeps if there are no waiting customers
// Since the barbID value is used so frequently to access indexes they will store the
//   actual index value at runtime and add +1 during printouts.
//   Meaning barbID runtime value will be 1 less than what is output
// Haircut service is the time between this method call and byeCustomer(int) call
//  
// pre: barbID >= 0
// param: barbID  ID value to be used for this barber thread
// post: Haircut service begins for the customer in this barber's chair
//
void Shop::helloCustomer(int barbID)
{
   pthread_mutex_lock(&mutex_);

   // If no customers then barber can sleep
   if (waiting_customers_ == 0 && customer_in_chair_[barbID] == 0) {
      print(-(barbID + 1), "sleeps because of no customers.");
      sleeping_barbs_++;
      pthread_cond_wait(&cond_barber_sleeping_[barbID], &mutex_);
   }

   while (customer_in_chair_[barbID] == 0)                     // Check if a customer sat down
   {
      pthread_cond_wait(&cond_barber_sleeping_[barbID], &mutex_);
   }

   print(-(barbID + 1), "starts a hair-cut service for customer[" 
                         + int2string(customer_in_chair_[barbID]) 
                         + string("]"));

   pthread_mutex_unlock(&mutex_);
}

// --------------------------- void byeCustomer(int)
// Second barber method.
// Uses mutex start to finish with a wait call for a customer to pay
// Completes the haircut service then requests payment from customer
// Once he receives confirmation the barber clears his chair and signals
//   another customer to sit
// 
// pre: A customer is waiting for this barber's haircut to finish
// param: barbID  ID value of this barber thread
// post: Customer associated with this barber is released and a new one is signaled
//
void Shop::byeCustomer(int barbID)
{
   pthread_mutex_lock(&mutex_); // lock

   // Hair Cut-Service is done so signal customer and wait for payment
   in_service_[barbID] = false;
   print(-(barbID + 1), "says he's done with a hair-cut service for customer[" 
                         + int2string(customer_in_chair_[barbID]) 
                         + string("]"));
   money_paid_[barbID] = false;

   pthread_cond_signal(&cond_customer_served_[barbID]);        // Signal customer to pay for haircut
   pthread_cond_wait(&cond_barber_paid_[barbID], &mutex_);

   //Signal to customer to get next one
   customer_in_chair_[barbID] = 0;
   print(-(barbID + 1), "calls in another customer");
   sleeping_barbs_++;
   pthread_cond_signal(&cond_customers_waiting_);

   pthread_mutex_unlock(&mutex_);  // unlock
}

// --------------------------- int get_cust_drops()
// pre: None
// return: cust_drops_
//
int Shop::get_cust_drops() const
{
   return cust_drops_;
}
