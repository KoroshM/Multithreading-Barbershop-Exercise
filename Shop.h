/** @file Shop.h
 * @author Korosh Moosavi
 * @date 2021-05-10
 * 
 * Modified version of shop_org.h provided by Prof. Dimpsey
 * 
 * Shop.h file:
 * All implementation is in the .cpp file
 * This header file includes the definitions for default numbers
 *   of chairs and barbers.
 * 
 * The Shop class is a monitor for barber and customer threads
 * Shop coordinates the interactions between these threads
 *   using various signals, condition variables, and a mutex
 * 
 * Assumptions:
 * The driver using this class calls the below methods in an appropriate
 *   order (i.e. helloCustomer() -> byeCustomer() )
 * Driver validates parameters before passing them into these methods
 */

#ifndef Shop_H_
#define Shop_H_
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#define kDefaultNumChairs 3 	// the default number of chairs for waiting = 3 
#define kDefaultBarbers 1  // the default number of barbers = 1 

class Shop
{
public:
   // --------------------------- Parameter constructor
   // Uses init() to initialize all array values and mutex/conditions
   // Sets max barbers and chairs to default values if parameter is invalid
   // 
   // pre: None
   // param: num_barbers  Maximum allowed number of barbers working at a time
   // param: num_chairs   Maximum number of waiting customers
   // post: Shop object can be passed threads of customers and barbers
   //
   Shop(int num_barbers, int num_chairs);

   // --------------------------- Default constructor
   // Uses init() to initialize all array values and mutex/conditions
   // Sets max barbers and chairs to default values
   // 
   // pre: None
   // post: Shop object can be passed threads of customers and barbers
   //
   Shop();

   // --------------------------- Destructor
   // Deletes the dynamically allocated arrays in this class (6 total)
   //
   ~Shop();

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
   int visitShop(int custID);

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
   void leaveShop(int custID, int barberID);

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
   void helloCustomer(int barbID);
   
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
   void byeCustomer(int barbID);
   
   // --------------------------- int get_cust_drops()
   // pre: None
   // return: cust_drops_
   //
   int get_cust_drops() const;

private:
   const int max_waiting_cust_;              // Max number of threads that can wait
   const int max_working_barb_;              // Max number of barbers
   int waiting_customers_;                   // Current number of occupied waiting chairs
   int sleeping_barbs_;                      // Currently available barbers
   int cust_drops_;                          // Number of missed customers because shop was full
   int* customer_in_chair_;                  // Array of customer IDs for barber chairs
   bool* in_service_;                        // Array of barber chair usage bools
   bool* money_paid_;                        // Array of bools for final transaction

   // Mutexes and condition variables to coordinate threads
   // mutex_ is used in conjuction with all conditional variables
   pthread_mutex_t mutex_;
   pthread_cond_t  cond_customers_waiting_;  // For barbers to signal customers in waiting chairs

   // Array of onditions for each barber
   pthread_cond_t* cond_customer_served_;    // For barber's final transaction
   pthread_cond_t* cond_barber_paid_;        // For customer's final transaction
   pthread_cond_t* cond_barber_sleeping_;    // For barber's sleep

   // --------------------------- void init()
   // Initializes every element of every array used in this class
   //   as well as the mutex and condition variables
   // 
   // pre: None
   // post: This Shop object is initialized and ready for operation
   //
   void init();
   
   // --------------------------- string int2string(int)
   // Uses a stringstream to convert an int into a string
   // 
   // pre: None
   // param: i  int to be converted
   // return: string containing i
   //
   string int2string(int i);
   
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
   void print(int person, string message);
   
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
   int assignBarber(int custID);
};
#endif
