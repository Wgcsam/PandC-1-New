#define mailbox_c

#include "mailbox.h"

#define NO_MAILBOXES 30

static void *shared_memory = NULL;
static mailbox *freelist = NULL;  /* list of free mailboxes.  */


/*
 *  initialise the data structures of mailbox.  Assign prev to the
 *  mailbox prev field.
 */

static mailbox *mailbox_config (mailbox *mbox, mailbox *prev)
{
  //mbox->data.result = 0; 		/* deprecated */
  //mbox->data.move_no = 0;		/* deprecated */
  //mbox->data.positions_explored = 0;	/* deprecated */
  mbox->prev = prev;							/* enables selection of previous mailbox */
  mbox->item_available = multiprocessor_initSem (0);			/* initializing semaphore */
  mbox->space_available = multiprocessor_initSem (MAX_MAILBOX_DATA);	/* initializing semaphore */
  mbox->mutex = multiprocessor_initSem (1);				/* initializing semaphore */
  mbox->in = 0;
  mbox->out = 0;
  return mbox;
}


/*
 *  init_memory - initialise the shared memory region once.
 *                It also initialises all mailboxes.
 */

static void init_memory (void)
{
  if (shared_memory == NULL)
    {
      mailbox *mbox;
      mailbox *prev = NULL;
      int i;
      _M2_multiprocessor_init ();
      shared_memory = multiprocessor_initSharedMemory
	(NO_MAILBOXES * sizeof (mailbox));
      mbox = shared_memory;
      for (i = 0; i < NO_MAILBOXES; i++)
	prev = mailbox_config (&mbox[i], prev);
      freelist = prev;
    }
}


/*
 *  init - create a single mailbox which can contain a single triple.
 */

mailbox *mailbox_init (void)
{
  mailbox *mbox;

  init_memory ();
  if (freelist == NULL)
    {
      printf ("exhausted mailboxes\n");
      exit (1);
    }
  mbox = freelist;
  freelist = freelist->prev;
  return mbox;
}


/*
 *  kill - return the mailbox to the freelist.  No process must use this
 *         mailbox.
 */

mailbox *mailbox_kill (mailbox *mbox)
{
  mbox->prev = freelist;
  freelist = mbox;
  return NULL;
}


/*
 *  send - send (result, move_no, positions_explored) to the mailbox mbox.
 */

void mailbox_send (mailbox *mbox, int result, int move_no, int positions_explored)
{
  multiprocessor_wait (mbox->space_available); 				/* wait for space available semaphore */
  multiprocessor_wait (mbox->mutex);					/* wait for mutex semaphore */
  //add item to buffer
  mbox->data[mbox->in].result = result;					/* add result to buffer */
  mbox->data[mbox->in].move_no = move_no;				/* add move_no to buffer */
  mbox->data[mbox->in].positions_explored = positions_explored;		/* add positions_explored to buffer */
  mbox->in = (mbox->in + 1) % MAX_MAILBOX_DATA;				/* increment in upto 100 and reset */
  multiprocessor_signal (mbox->mutex);					/* signal mutex semaphore */
  multiprocessor_signal (mbox->item_available);				/* signal item available semaphore */
}


/*
 *  rec - receive (result, move_no, positions_explored) from the
 *        mailbox mbox.
 */

void mailbox_rec (mailbox *mbox,
		  int *result, int *move_no, int *positions_explored)
{
  multiprocessor_wait (mbox->item_available);				/* wait for item available semaphore */
  multiprocessor_wait (mbox->mutex);					/* wait for mutex semaphore */
  //remove item from buffer to nextc
  *result = mbox->data[mbox->out].result;				/* receive result from buffer and pass back */
  *move_no = mbox->data[mbox->out].move_no;				/* receive move_no from buffer and pass back */
  *positions_explored = mbox->data[mbox->out].positions_explored;	/* receive positions_explored from buffer and pass back */
  mbox->out = (mbox->out + 1) % MAX_MAILBOX_DATA;			/* increment out upto 100 and reset */
  multiprocessor_signal (mbox->mutex);					/* signal mutex semaphore */
  multiprocessor_signal (mbox->space_available);			/* signal space available semaphore */
}
