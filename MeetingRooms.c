// Task requirements
// System to manage meeting rooms
// Each meeting have start time and end time
// Implement the below functions
// Only a single thread will use this system in any given time -- bonus: support multiple threads


#include <stdio.h>
#include <time.h>
#include <assert.h>
//#include <pthread.h>





#define MAX_ROOM_NUM 2056           // ~(2^11)
#define MEETING_CAPACITY 1073741824 // (2^30) Size of the Hash Table
#define MINUTE 60
#define HOUR 60 * 60
#define DAY 24 * 60 * 60
#define MONTH 30 * 24 * 60 * 60
#define YEAR 365 * 24 * 60 * 60
//#define MIN_TIME 0
//#define MAX_TIME 2147483647 // 2^31-1
// #define DEFINED_MIN_INTERVAL 60 // Meeting is minimum one minute
#define MAX_MEETING_IDS 2147483647


// Possible to make it const after debugging 
time_t MIN_TIME = MONTH;      // In the past
time_t MAX_TIME = 25 * YEAR;  // In the future
time_t MIN_INTERVAL = MINUTE; // Between start to end
int ACTIVE_ROOMS = 0;


void free_all();
void checks();

enum nodeColor
{
  RED,
  BLACK
};

enum tbst_tag 
  {
    TBST_CHILD,  /* Child pointer. */
    TBST_THREAD  /* Thread. */
  };


struct LinkedList
{
  unsigned int val;
  struct LinkedList *next;
};
struct LinkedList *MEETING_ID_LEFT = NULL;


void free_linked_list(){
  struct LinkedList *meeting_id_child_nodeed_list_node = NULL;
  meeting_id_child_nodeed_list_node = MEETING_ID_LEFT;
  while(meeting_id_child_nodeed_list_node != NULL){
    MEETING_ID_LEFT = MEETING_ID_LEFT->next;
    free(meeting_id_child_nodeed_list_node);
    meeting_id_child_nodeed_list_node = MEETING_ID_LEFT;
  }
}

/*
*  Structure: rb_node_rooms
*  ----------------------------
*  Nodes of RB Tree for storing meetings.
*  
*  Param:
*  start_time
*  end_time
*  color - (0/1 marks R or B)
*  child_node[2] - [0] left child and [1] right child
*  threaded - Connection between predecessor and successor values
*/
struct rb_node_rooms
{
  time_t start_time;
  time_t end_time;
  char color;
  struct rb_node_rooms *child_node[2];
  struct rb_node_rooms *threaded[2];
};
struct rb_node_rooms *root = NULL;
struct rb_node_rooms *NIL_THREADED = NULL;


/*
*  Structure: Tree
*  ----------------------------
*  Tree for storing rb_node_rooms nodes root level. This is the root manager per room.
*  
*  Param:
*  count - total nodes count
*  room - room_id
*  root - pointer to root
*  min - min value stored (meeting start time)
*  max - max value stored
*/
typedef struct Tree
{
  int count;
  int room;
  struct rb_node_rooms *root;
  time_t min;
  time_t max;
} Tree;
Tree *ROOMS;


/*
*  Structure: rb_node_meeting_id
*  ----------------------------
*  Nodes of RB Tree for storing meeting_id.
*  Similar to rb_node_rooms but not threaded
*  
*  Param:
*  meeting_id
*  room_id
*  color - (0/1 marks R or B)
*  room_node - pointer to rb_node_rooms root node
*  child_node[2] - [0] left child and [1] right child
*/
struct rb_node_meeting_id
{
  unsigned int meeting_id;
  unsigned int room_id;
  char color;
  struct rb_node_rooms *room_node;
  struct rb_node_meeting_id *child_node[2];
};
struct rb_node_meeting_id *meetings_root = NULL;



/*
*  Function: fix_min_max_room_tree
*  ----------------------------
*  Updates the monitor tree in case one of the parameter values was removed from the tree
*  
*/
void fix_min_max_room_tree(unsigned int room_id, time_t start_time, time_t end_time){
  struct rb_node_rooms *tmp_node;
  if (ROOMS[room_id].max == end_time)
  {
    tmp_node = ROOMS[room_id].root;
    if (tmp_node == NULL) ROOMS[room_id].max = MIN_TIME;
    else{
      while (tmp_node->child_node[1] != NULL) tmp_node = tmp_node->child_node[1];
      ROOMS[room_id].max = tmp_node->end_time;
    }
  }
  if (ROOMS[room_id].max == start_time){
    struct rb_node_rooms *tmp_node;
    tmp_node = ROOMS[room_id].root;
    if (tmp_node == NULL) ROOMS[room_id].min = MAX_TIME;
    else{
      while (tmp_node->child_node[0] != NULL) tmp_node = tmp_node->child_node[0];
      ROOMS[room_id].min = tmp_node->start_time;
    }
  }
}


/*
*  Function: short_fix_min_max_room_tree
*  ----------------------------
*  Updates the monitor tree in case one of the old parameter values was removed from the tree
*  
*/
void short_fix_min_max_room_tree(unsigned int room_id, time_t old_start_time, time_t old_end_time,
                                                        time_t new_start_time, time_t new_end_time){
  if (ROOMS[room_id].min == old_start_time) ROOMS[room_id].min = new_start_time;
  if (ROOMS[room_id].max == old_end_time) ROOMS[room_id].max = new_end_time;
}


// Creates a room red-black tree
struct rb_node_rooms *create_node_rooms(time_t start_time, time_t end_time)
{
  struct rb_node_rooms *newnode;
  newnode = (struct rb_node_rooms *)malloc(sizeof(struct rb_node_rooms));
  if (newnode == NULL)
    return NULL;
  newnode->start_time = start_time;
  newnode->end_time = end_time;
  newnode->color = RED;
  newnode->child_node[0] = newnode->child_node[1] = NULL;
  newnode->threaded[0] = newnode->threaded[1] = NULL;
  return newnode;
}


/*
*  Function: find_node_rooms
*  ----------------------------
*  Find the location of a meeting node to a room tree, if possible 
* 
*  In
*  source - room_id source tree
*  start_time - meeting start time
*  end_time - meeting end time
*  error - pointer to error
* 
*  Out
*  source - fixed room_id source tree
*  error - exit code for the function
* 
*  Execution error coodes:
*  -31 Node in range already exists
*  -32 Node in range doesn't exist
*  -33 source input was NULL
*  -34 Node in range cannot exist
*/
void find_node_rooms(struct rb_node_rooms **source, struct rb_node_rooms **target_node, 
                      struct rb_node_rooms *stack[], int dir[], int *ht, int* child_dir,
                      time_t start_time, time_t end_time, int *error)
{
  *error = 0;

  if (*source == NULL){
    *error = 33;
    return;
  }

  struct rb_node_rooms *ptr;
  ptr = *source;

  stack[*ht] = *source;
  dir[(*ht)++] = 0;

  while (ptr != NULL)
  {
    *child_dir = (start_time >= ptr->end_time) ? 1 : 0;
    if (!(*child_dir)){
      *child_dir = (end_time <= ptr->start_time) ? 0 : 2;
      if (*child_dir == 2)
      {
        if (start_time == ptr->start_time && end_time == ptr->end_time)
          *error = -31;
        else
          *error = -34;
        *target_node = ptr;
        return;
      }
    }
    stack[*ht] = ptr;
    ptr = ptr->child_node[*child_dir];
    dir[(*ht)++] = *child_dir;
  }
  *target_node = ptr;
  *error = -32;
}


/*
 * Function: insertion_rooms
 * ----------------------------
 * Inserts a meeting node to a room tree, if possible 
 *
 * In
 * source - room_id source tree
 * start_time - meeting start time
 * end_time - meeting end time
 * error - pointer to error
 *
 * Out
 * source - fixed room_id source tree
 * added_node - added_node address 
 * error - exit code for the function
 *
 * Error coodes:
 * -31 Node in range already exists
 * -33 source input was NULL
 * -34 Node in range cannot exist
 * -21 new_node mem allocation failed
*/
void insertion_rooms(struct rb_node_rooms **source, struct rb_node_rooms **added_node, time_t start_time, time_t end_time, int *error)
{
  *error = 0;
  struct rb_node_rooms *newnode;
  
  if (*source == NULL)
  {
    newnode = create_node_rooms(start_time, end_time);
    if (newnode == NULL)
      *error = -21;
    else
      *source = newnode;
    *added_node = newnode;
    return;
  }

struct rb_node_rooms *stack[98], *ptr;
int dir[98], ht = 0, child_dir;

find_node_rooms(source, &ptr, stack, dir, &ht, &child_dir, start_time, end_time, error);
if (*error != -32) return;
*error = 0;

newnode = create_node_rooms(start_time, end_time);
if (newnode == NULL)
  *error = -21;

if (child_dir){
  newnode->threaded[0] = stack[ht - 1];
  newnode->threaded[1] = stack[ht - 1]->threaded[child_dir];
  if (stack[ht - 1]->threaded[1] != NULL)
    stack[ht - 1]->threaded[1]->threaded[0] = newnode;
  stack[ht - 1]->threaded[1] = newnode;
  }
else
{
  newnode->threaded[1] = stack[ht - 1];
  newnode->threaded[0] = stack[ht - 1]->threaded[0];
  if (stack[ht - 1]->threaded[0] != NULL)
    stack[ht - 1]->threaded[0]->threaded[1] = newnode;
  stack[ht - 1]->threaded[0] = newnode;
}
stack[ht - 1]->child_node[child_dir] = newnode;
*added_node = newnode;

struct rb_node_rooms *xPtr, *yPtr;
while ((ht >= 3) && (stack[ht - 1]->color == RED))
  {
    if (dir[ht - 2] == 0)
    {
      yPtr = stack[ht - 2]->child_node[1];
      if (yPtr != NULL && yPtr->color == RED)
      {
        stack[ht - 2]->color = RED;
        stack[ht - 1]->color = yPtr->color = BLACK;
        ht = ht - 2;
      }
      else
      {
        if (dir[ht - 1] == 0)
        {
          yPtr = stack[ht - 1];
        }
        else
        {
          xPtr = stack[ht - 1];
          yPtr = xPtr->child_node[1];
          xPtr->child_node[1] = yPtr->child_node[0];
          yPtr->child_node[0] = xPtr;
          stack[ht - 2]->child_node[0] = yPtr;
        }
        xPtr = stack[ht - 2];
        xPtr->color = RED;
        yPtr->color = BLACK;
        xPtr->child_node[0] = yPtr->child_node[1];
        yPtr->child_node[1] = xPtr;
        if (xPtr == *source)
        {
          *source = yPtr;
        }
        else
        {
          stack[ht - 3]->child_node[dir[ht - 3]] = yPtr;
        }
        break;
      }
    }
    else
    {
      yPtr = stack[ht - 2]->child_node[0];
      if ((yPtr != NULL) && (yPtr->color == RED))
      {
        stack[ht - 2]->color = RED;
        stack[ht - 1]->color = yPtr->color = BLACK;
        ht = ht - 2;
      }
      else
      {
        if (dir[ht - 1] == 1)
          yPtr = stack[ht - 1];
        else
        {
          xPtr = stack[ht - 1];
          yPtr = xPtr->child_node[0];
          xPtr->child_node[0] = yPtr->child_node[1];
          yPtr->child_node[1] = xPtr;
          stack[ht - 2]->child_node[1] = yPtr;
        }
        xPtr = stack[ht - 2];
        yPtr->color = BLACK;
        xPtr->color = RED;
        xPtr->child_node[1] = yPtr->child_node[0];
        yPtr->child_node[0] = xPtr;
        if (xPtr == *source)
          *source = yPtr;
        else
          stack[ht - 3]->child_node[dir[ht - 3]] = yPtr;
        break;
      }
    }
  }
  (*source)->color = BLACK;
}


/*
 * Function: deletion_rooms
 * ----------------------------
 * Deletes a node in the meeting_id tree
 *
 * In
 * source - room_id source tree
 * start_time - meeting start time
 * end_time - meeting end time
 * error - pointer to error
 *
 * Out
 * source - fixed room_id source tree
 * error - exit code for the function
 *
 * Error coodes:
 * -32 Node in range doesn't exist
 * -33 source input was NULL
 */
void deletion_rooms(struct rb_node_rooms **source, time_t start_time, time_t end_time, int *error)
{
  struct rb_node_rooms *stack[98], *ptr, *xPtr, *yPtr;
  struct rb_node_rooms *pPtr, *qPtr, *rPtr;
  int dir[98], ht = 0, diff, i;
  enum nodeColor color;

  if (*source == NULL)
  {
    *error = -33;
    return;
  }

  // Based on a similar implementation of GNU libavl (https://adtinfo.org/libavl.html/)

  ptr = *source;
  while (ptr != NULL)
  {
    if (start_time == ptr->start_time && end_time == ptr->end_time)
      break;

    diff = (start_time >= ptr->end_time) ? 1 : 0;
    if (!diff)
      diff = (end_time <= ptr->start_time) ? 0 : 2;
    if (diff == 2)
    {
      *error = -32;
      return;
    }

    stack[ht] = ptr;
    dir[ht++] = diff;
    ptr = ptr->child_node[diff];
  }

  if (ptr == NULL)
  {
    *error = 32;
    return;
  }

  if (ptr->threaded[1] != NULL && ptr->threaded[1]->threaded[0] != NULL)
    ptr->threaded[1]->threaded[0] = ptr->threaded[0];
  if (ptr->threaded[0] != NULL && ptr->threaded[0]->threaded[1] != NULL)
    ptr->threaded[0]->threaded[1] = ptr->threaded[1];

  if (ptr->child_node[1] == NULL)
  {
    if (ptr == *source && ptr->child_node[0] == NULL)
    {
      free(ptr);
      *source = NULL;
    }
    else if (ptr == *source)
    {
      *source = ptr->child_node[0];
      free(ptr);
    }
    else
    {
      stack[ht - 1]->child_node[dir[ht - 1]] = ptr->child_node[0];
    }
  }
  else
  {
    xPtr = ptr->child_node[1];
    if (xPtr->child_node[0] == NULL)
    {
      xPtr->child_node[0] = ptr->child_node[0];
      color = xPtr->color;
      xPtr->color = ptr->color;
      ptr->color = color;

      if (ptr == *source)
      {
        *source = xPtr;
      }
      else
      {
        stack[ht - 1]->child_node[dir[ht - 1]] = xPtr;
      }

      dir[ht] = 1;
      stack[ht++] = xPtr;
    }
    else
    {
      i = ht++;
      while (1)
      {
        dir[ht] = 0;
        stack[ht++] = xPtr;
        yPtr = xPtr->child_node[0];
        if (!yPtr->child_node[0])
          break;
        xPtr = yPtr;
      }

      dir[i] = 1;
      stack[i] = yPtr;
      if (i > 0)
        stack[i - 1]->child_node[dir[i - 1]] = yPtr;

      yPtr->child_node[0] = ptr->child_node[0];

      xPtr->child_node[0] = yPtr->child_node[1];
      yPtr->child_node[1] = ptr->child_node[1];

      if (ptr == *source)
      {
        *source = yPtr;
      }

      color = yPtr->color;
      yPtr->color = ptr->color;
      ptr->color = color;
    }
  }

  if (ht < 1)
    return;

  if (ptr->color == BLACK)
  {
    while (1)
    {
      pPtr = stack[ht - 1]->child_node[dir[ht - 1]];
      if (pPtr && pPtr->color == RED)
      {
        pPtr->color = BLACK;
        break;
      }

      if (ht < 2)
        break;

      if (dir[ht - 2] == 0)
      {
        rPtr = stack[ht - 1]->child_node[1];

        if (!rPtr)
          break;

        if (rPtr->color == RED)
        {
          stack[ht - 1]->color = RED;
          rPtr->color = BLACK;
          stack[ht - 1]->child_node[1] = rPtr->child_node[0];
          rPtr->child_node[0] = stack[ht - 1];

          if (stack[ht - 1] == *source)
          {
            *source = rPtr;
          }
          else
          {
            stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
          }
          dir[ht] = 0;
          stack[ht] = stack[ht - 1];
          stack[ht - 1] = rPtr;
          ht++;

          rPtr = stack[ht - 1]->child_node[1];
        }

        if ((!rPtr->child_node[0] || rPtr->child_node[0]->color == BLACK) &&
            (!rPtr->child_node[1] || rPtr->child_node[1]->color == BLACK))
        {
          rPtr->color = RED;
        }
        else
        {
          if (!rPtr->child_node[1] || rPtr->child_node[1]->color == BLACK)
          {
            qPtr = rPtr->child_node[0];
            rPtr->color = RED;
            qPtr->color = BLACK;
            rPtr->child_node[0] = qPtr->child_node[1];
            qPtr->child_node[1] = rPtr;
            rPtr = stack[ht - 1]->child_node[1] = qPtr;
          }
          rPtr->color = stack[ht - 1]->color;
          stack[ht - 1]->color = BLACK;
          rPtr->child_node[1]->color = BLACK;
          stack[ht - 1]->child_node[1] = rPtr->child_node[0];
          rPtr->child_node[0] = stack[ht - 1];
          if (stack[ht - 1] == *source)
          {
            *source = rPtr;
          }
          else
          {
            stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
          }
          break;
        }
      }
      else
      {
        rPtr = stack[ht - 1]->child_node[0];

        if (!rPtr)
          break;
        if (rPtr->color == RED)
        {
          stack[ht - 1]->color = RED;
          rPtr->color = BLACK;
          stack[ht - 1]->child_node[0] = rPtr->child_node[1];
          rPtr->child_node[1] = stack[ht - 1];

          if (stack[ht - 1] == *source)
          {
            *source = rPtr;
          }
          else
          {
            stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
          }
          dir[ht] = 1;
          stack[ht] = stack[ht - 1];
          stack[ht - 1] = rPtr;
          ht++;

          rPtr = stack[ht - 1]->child_node[0];
        }
        if (rPtr)
        {
          if ((!rPtr->child_node[0] || rPtr->child_node[0]->color == BLACK) &&
              (!rPtr->child_node[1] || rPtr->child_node[1]->color == BLACK))
          {
            rPtr->color = RED;
          }
          else
          {
            if (!rPtr->child_node[0] || rPtr->child_node[0]->color == BLACK)
            {
              qPtr = rPtr->child_node[1];
              rPtr->color = RED;
              qPtr->color = BLACK;
              rPtr->child_node[1] = qPtr->child_node[0];
              qPtr->child_node[0] = rPtr;
              rPtr = stack[ht - 1]->child_node[0] = qPtr;
            }
            rPtr->color = stack[ht - 1]->color;
            stack[ht - 1]->color = BLACK;
            rPtr->child_node[0]->color = BLACK;
            stack[ht - 1]->child_node[0] = rPtr->child_node[1];
            rPtr->child_node[1] = stack[ht - 1];
            if (stack[ht - 1] == *source)
            {
              *source = rPtr;
            }
            else
            {
              stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
            }
            break;
          }
        }
      }
      ht--;
    }
  }
}


/*
 * Function: inorder_traversal_rooms
 * ----------------------------
 *	Print the inorder traversal of a room tree
 *
 *	In:
 *	node: Root rb_node_rooms node to traversal
 */
void inorder_traversal_rooms(struct rb_node_rooms *node)
{
  if (node != NULL)
  {
    inorder_traversal_rooms(node->child_node[0]);
    printf("%lld  ", node->start_time);
    inorder_traversal_rooms(node->child_node[1]);
  }
  return;
}

void print_all_trees_rooms()
{
  int i = 0;
  for (; i < ACTIVE_ROOMS; i++)
  {
    printf("ROOM: %d \n", i);
    inorder_traversal_rooms(ROOMS[i].root);
    printf("\n");
  }
}

/*
 * Function: delete_tree_helper_rooms
 * ----------------------------
 *	Helper function for delete_tree_rooms
 *
 *	In:
 *	node: Root rb_node_rooms node to traversal
 */
void delete_tree_helper_rooms(struct rb_node_rooms *node)
{
  if (node->child_node[0] != NULL)
    delete_tree_helper_rooms(node->child_node[0]);
  if (node->child_node[1] != NULL)
    delete_tree_helper_rooms(node->child_node[1]);
  free(node);
}

/*
 * Function: delete_tree_rooms
 * ----------------------------
 *	Free the memory of a room tree
 *
 *	In:
 *	node: Root rb_node_rooms node to traversal
 */
void delete_tree_rooms(struct rb_node_rooms *source)
{
  if (source == NULL) return;
  delete_tree_helper_rooms(source);
}


/*
 * Function: rb_node_meeting_id
 * ----------------------------
 * Creates a new red-black node for meeting_id tree
 *
 * In
 * meeting_id - Tree key
 * room_id
 * room_node -pointer to node in the room 
 *
 * Return
 * newnode node or NULL if memory allocation failed
 */

struct rb_node_meeting_id *create_node_meeting_id(unsigned int meeting_id, unsigned int room_id, struct rb_node_rooms *room_node)
{
  struct rb_node_meeting_id *newnode;
  newnode = (struct rb_node_meeting_id *)malloc(sizeof(struct rb_node_meeting_id));
  if (newnode == NULL)
    return NULL;
  newnode->meeting_id = meeting_id;
  newnode->room_id = room_id;
  newnode->color = RED;
  newnode->room_node = room_node;
  newnode->child_node[0] = newnode->child_node[1] = NULL;
  return newnode;
}

/*
 * Function: insertion_meeting_id
 * ----------------------------
 * Insert a node in the meeting_id tree
 *
 * In:
 * source - fixed meeting_id source tree
 * meeting_id - meeting_id to insert (key)
 * room_id - room_id of the tree
 * error
 *
 * Out:
 * source - fixed meeting_id source tree
 * error - exit code for the function
 *
 * Error coodes:
 * -51 Meeting ID in range already exists
 * -21 new_node maloc failed
 */
void insertion_meeting_id(struct rb_node_meeting_id **source, struct rb_node_rooms *room_node, unsigned int meeting_id, unsigned int room_id, int *error)
{
  *error = 0;
  struct rb_node_meeting_id *stack[98], *ptr, *newnode, *xPtr, *yPtr;
  int dir[98], ht = 0, index;
  ptr = *source;

  if (ptr == NULL)
  {
    newnode = create_node_meeting_id(meeting_id, room_id, room_node);
    if (newnode == NULL)
      *error = -21;
    else
      *source = newnode;
    return;
  }
  stack[ht] = *source;
  dir[ht++] = 0;
  while (ptr != NULL)
  {
    if (meeting_id == ptr->meeting_id)
    {
      *error = -51;
      return;
    }
    index = (meeting_id > ptr->meeting_id) ? 1 : 0;
    stack[ht] = ptr;
    ptr = ptr->child_node[index];
    dir[ht++] = index;
  }
  newnode = create_node_meeting_id(meeting_id, room_id, room_node);
  if (newnode == NULL)
    *error = -21;
  stack[ht - 1]->child_node[index] = newnode;

  while ((ht >= 3) && (stack[ht - 1]->color == RED))
  {
    if (dir[ht - 2] == 0)
    {
      yPtr = stack[ht - 2]->child_node[1];
      if (yPtr != NULL && yPtr->color == RED)
      {
        stack[ht - 2]->color = RED;
        stack[ht - 1]->color = yPtr->color = BLACK;
        ht = ht - 2;
      }
      else
      {
        if (dir[ht - 1] == 0)
        {
          yPtr = stack[ht - 1];
        }
        else
        {
          xPtr = stack[ht - 1];
          yPtr = xPtr->child_node[1];
          xPtr->child_node[1] = yPtr->child_node[0];
          yPtr->child_node[0] = xPtr;
          stack[ht - 2]->child_node[0] = yPtr;
        }
        xPtr = stack[ht - 2];
        xPtr->color = RED;
        yPtr->color = BLACK;
        xPtr->child_node[0] = yPtr->child_node[1];
        yPtr->child_node[1] = xPtr;
        if (xPtr == *source)
        {
          *source = yPtr;
        }
        else
        {
          stack[ht - 3]->child_node[dir[ht - 3]] = yPtr;
        }
        break;
      }
    }
    else
    {
      yPtr = stack[ht - 2]->child_node[0];
      if ((yPtr != NULL) && (yPtr->color == RED))
      {
        stack[ht - 2]->color = RED;
        stack[ht - 1]->color = yPtr->color = BLACK;
        ht = ht - 2;
      }
      else
      {
        if (dir[ht - 1] == 1)
          yPtr = stack[ht - 1];
        else
        {
          xPtr = stack[ht - 1];
          yPtr = xPtr->child_node[0];
          xPtr->child_node[0] = yPtr->child_node[1];
          yPtr->child_node[1] = xPtr;
          stack[ht - 2]->child_node[1] = yPtr;
        }
        xPtr = stack[ht - 2];
        yPtr->color = BLACK;
        xPtr->color = RED;
        xPtr->child_node[1] = yPtr->child_node[0];
        yPtr->child_node[0] = xPtr;
        if (xPtr == *source)
          *source = yPtr;
        else
          stack[ht - 3]->child_node[dir[ht - 3]] = yPtr;
        break;
      }
    }
  }
  (*source)->color = BLACK;
}


/*
* Function: find_meeting_id_node
* ----------------------------
* Finds the node in a the meeting_id tree
*
* In:
* source - meeting_id source tree
* meeting_id - id to find
*
* Out:
* target_node - fixed meeting_id source tree
* stack - Path to the node
* dir - Direction taken from path[i]
* ht - node height
* error
* 
* Error coodes:
* -52 Node in range doesn't exist
* -53 source input was NULL
*/
void find_meeting_id_node(struct rb_node_meeting_id **source, struct rb_node_meeting_id **target_node, 
                          struct rb_node_meeting_id *stack[], int dir[], int *ht,
                          unsigned int meeting_id, int *error)
{
  struct rb_node_meeting_id *ptr;
  int child_dir;
  *ht = 0;
  *error = 0;

  ptr = *source;
  if (*source == NULL)
    *error = -53;
  else{
    while (ptr != NULL)
    {
      if (meeting_id == ptr->meeting_id)
        break;
      child_dir = (meeting_id > ptr->meeting_id) ? 1 : 0;
      stack[*ht] = ptr;
      dir[(*ht)++] = child_dir;
      ptr = ptr->child_node[child_dir];
    }
    if (ptr == NULL)
      *error = 52;
  }

  *target_node = ptr;
}

/*
 * Function: deletion_meeting_id
 * ----------------------------
 * Deletes a node in the meeting_id tree
 *
 * In:
 * source - meeting_id source tree
 * meeting_id - meeting_id to delete
 * error
 *
 * Out:
 * source - fixed meeting_id source tree
 * error - exit code for the function
 *
 * Error coodes:
 * -52 Node in range doesn't exist
 * -53 source input was NULL
 */
void deletion_meeting_id(struct rb_node_meeting_id **source, unsigned int meeting_id, int *error, 
                        struct rb_node_rooms **room_node, unsigned int *room_id)
{
  *error = 0;
  struct rb_node_meeting_id *stack[98], *ptr;
  struct rb_node_meeting_id *xPtr, *yPtr, *pPtr, *qPtr, *rPtr;
  int dir[98], ht, i;
  enum nodeColor color;

  find_meeting_id_node(source, &ptr, stack, dir, &ht, meeting_id, error);
  if (*error) return;

  if (room_node != NULL)
    *room_node = ptr->room_node;
  if (room_id != NULL)
    *room_id = ptr->room_id;

  if (ptr->child_node[1] == NULL)
  {
    if (ptr == *source && ptr->child_node[0] == NULL)
    {
      free(ptr);
      *source = NULL;
    }
    else if (ptr == *source)
    {
      *source = ptr->child_node[0];
      free(ptr);
    }
    else
    {
      stack[ht - 1]->child_node[dir[ht - 1]] = ptr->child_node[0];
    }
  }
  else
  {
    xPtr = ptr->child_node[1];
    if (xPtr->child_node[0] == NULL)
    {
      xPtr->child_node[0] = ptr->child_node[0];
      color = xPtr->color;
      xPtr->color = ptr->color;
      ptr->color = color;

      if (ptr == *source)
      {
        *source = xPtr;
      }
      else
      {
        stack[ht - 1]->child_node[dir[ht - 1]] = xPtr;
      }

      dir[ht] = 1;
      stack[ht++] = xPtr;
    }
    else
    {
      i = ht++;
      while (1)
      {
        dir[ht] = 0;
        stack[ht++] = xPtr;
        yPtr = xPtr->child_node[0];
        if (!yPtr->child_node[0])
          break;
        xPtr = yPtr;
      }

      dir[i] = 1;
      stack[i] = yPtr;
      if (i > 0)
        stack[i - 1]->child_node[dir[i - 1]] = yPtr;

      yPtr->child_node[0] = ptr->child_node[0];

      xPtr->child_node[0] = yPtr->child_node[1];
      yPtr->child_node[1] = ptr->child_node[1];

      if (ptr == *source)
      {
        *source = yPtr;
      }

      color = yPtr->color;
      yPtr->color = ptr->color;
      ptr->color = color;
    }
  }

  if (ht < 1)
    return;

  if (ptr->color == BLACK)
  {
    while (1)
    {
      pPtr = stack[ht - 1]->child_node[dir[ht - 1]];
      if (pPtr && pPtr->color == RED)
      {
        pPtr->color = BLACK;
        break;
      }

      if (ht < 2)
        break;

      if (dir[ht - 2] == 0)
      {
        rPtr = stack[ht - 1]->child_node[1];

        if (!rPtr)
          break;

        if (rPtr->color == RED)
        {
          stack[ht - 1]->color = RED;
          rPtr->color = BLACK;
          stack[ht - 1]->child_node[1] = rPtr->child_node[0];
          rPtr->child_node[0] = stack[ht - 1];

          if (stack[ht - 1] == *source)
          {
            *source = rPtr;
          }
          else
          {
            stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
          }
          dir[ht] = 0;
          stack[ht] = stack[ht - 1];
          stack[ht - 1] = rPtr;
          ht++;

          rPtr = stack[ht - 1]->child_node[1];
        }

        if ((!rPtr->child_node[0] || rPtr->child_node[0]->color == BLACK) &&
            (!rPtr->child_node[1] || rPtr->child_node[1]->color == BLACK))
        {
          rPtr->color = RED;
        }
        else
        {
          if (!rPtr->child_node[1] || rPtr->child_node[1]->color == BLACK)
          {
            qPtr = rPtr->child_node[0];
            rPtr->color = RED;
            qPtr->color = BLACK;
            rPtr->child_node[0] = qPtr->child_node[1];
            qPtr->child_node[1] = rPtr;
            rPtr = stack[ht - 1]->child_node[1] = qPtr;
          }
          rPtr->color = stack[ht - 1]->color;
          stack[ht - 1]->color = BLACK;
          rPtr->child_node[1]->color = BLACK;
          stack[ht - 1]->child_node[1] = rPtr->child_node[0];
          rPtr->child_node[0] = stack[ht - 1];
          if (stack[ht - 1] == *source)
          {
            *source = rPtr;
          }
          else
          {
            stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
          }
          break;
        }
      }
      else
      {
        rPtr = stack[ht - 1]->child_node[0];
        if (!rPtr)
          break;

        if (rPtr->color == RED)
        {
          stack[ht - 1]->color = RED;
          rPtr->color = BLACK;
          stack[ht - 1]->child_node[0] = rPtr->child_node[1];
          rPtr->child_node[1] = stack[ht - 1];

          if (stack[ht - 1] == *source)
          {
            *source = rPtr;
          }
          else
          {
            stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
          }
          dir[ht] = 1;
          stack[ht] = stack[ht - 1];
          stack[ht - 1] = rPtr;
          ht++;

          rPtr = stack[ht - 1]->child_node[0];
        }
        if (rPtr)
        {
          if ((!rPtr->child_node[0] || rPtr->child_node[0]->color == BLACK) &&
              (!rPtr->child_node[1] || rPtr->child_node[1]->color == BLACK))
          {
            rPtr->color = RED;
          }
          else
          {
            if (!rPtr->child_node[0] || rPtr->child_node[0]->color == BLACK)
            {
              qPtr = rPtr->child_node[1];
              rPtr->color = RED;
              qPtr->color = BLACK;
              rPtr->child_node[1] = qPtr->child_node[0];
              qPtr->child_node[0] = rPtr;
              rPtr = stack[ht - 1]->child_node[0] = qPtr;
            }
            rPtr->color = stack[ht - 1]->color;
            stack[ht - 1]->color = BLACK;
            rPtr->child_node[0]->color = BLACK;
            stack[ht - 1]->child_node[0] = rPtr->child_node[1];
            rPtr->child_node[1] = stack[ht - 1];
            if (stack[ht - 1] == *source)
            {
              *source = rPtr;
            }
            else
            {
              stack[ht - 2]->child_node[dir[ht - 2]] = rPtr;
            }
            break;
          }
        }
      }
      ht--;
    }
  }
}

/*
 * Function: inorder_traversal_meeting_id
 * ----------------------------
 *	Print the inorder traversal of the meeting_id tree
 *
 *	In:
 *	node: Root rb_node_meeting_id node to traversal
 */
void inorder_traversal_meeting_id(struct rb_node_meeting_id *node)
{
  if (node != NULL)
  {
    inorder_traversal_meeting_id(node->child_node[0]);
    printf("%d  ", node->meeting_id);
    inorder_traversal_meeting_id(node->child_node[1]);
  }
  return;
}

/*
 * Function: delete_tree_helper_meeting_id
 * ----------------------------
 *	Helper function for delete_tree_meeting_id
 *
 *	In:
 *	node: Root rb_node_meeting_id node to traversal
 */
void delete_tree_helper_meeting_id(struct rb_node_meeting_id *node)
{
  if (node->child_node[0] != NULL)
    delete_tree_helper_meeting_id(node->child_node[0]);
  if (node->child_node[1] != NULL)
    delete_tree_helper_meeting_id(node->child_node[1]);
  free(node);
}

/*
 * Function: delete_tree_meeting_id
 * ----------------------------
 *	Free the memory of meeting_id tree
 *
 *	In:
 *	node: Root rb_node_meeting_id node to traversal
 */
void delete_tree_meeting_id(struct rb_node_meeting_id *source)
{
  if (source == NULL) return;
  delete_tree_helper_meeting_id(source);
}

/*
 * Function: delete_all_trees
 * ----------------------------
 *	Free the memory of all trees
 */
void delete_all_trees()
{
  int i = 0;
  for (; i < ACTIVE_ROOMS; i++)
  {
    printf("Tree: %d \n", i);
    delete_tree_rooms(ROOMS[i].root);
  }
  delete_tree_meeting_id(meetings_root);
}

/*
 * Function: print_time_interval
 * ----------------------------
 * Prints the time of two time_t parameters
 *
 *	In:
 *	start_time: first item to print
 * end_time: second item to print
 */
void print_time_interval(time_t start_time, time_t end_time)
{
  struct tm *ts;
  char buf[80];

  ts = localtime(&start_time);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ts);
  puts(buf);

  ts = localtime(&end_time);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ts);
  puts(buf);
}

/*
 * Function: check_time
 * ----------------------------
 * Helper function to
 *
 *	In:
 *	start_time: first item to print
 * end_time: second item to print
 *
 *	Out:
 * 0 if adding meeting succeeded, a negative number representing the error otherwise
 *	-11 If the meeting starting time is after the meeting ending time
 * -12 If the meeting starting time is too much in the past
 * -13 If the minimum time interval is lower than defined (e.g. less than 1 minute = 60)
 */
int check_time(time_t start_time, time_t end_time)
{
  time_t now;
  now = time(NULL);
  if (end_time < start_time)
    return -11;
  if (start_time < MIN_TIME)
    return -12;
  if (end_time - start_time < MIN_INTERVAL)
    return -13;
  return 0;
}

/*
 * Function: meeting_system_init
 * ----------------------------
 *	Initializes meeting system
 *
 *	In:
 *	meeting_room_num: number of meeting rooms for the system to manage
 *
 *	Return value:
 *	0 if initialization succeeded, a negative number representing the error otherwise
 *  -14 if meeting_room_num is bigger than the defined maximum
 */
int meeting_system_init(int meeting_room_num)
{
  if (meeting_room_num > MAX_ROOM_NUM) return -1; // wrong input
  free_all(); // If init again

  time_t now;
  now = time(NULL);
  MIN_TIME = now - MIN_TIME;
  MAX_TIME = now + MAX_TIME;

  ROOMS = (Tree *)malloc(meeting_room_num * sizeof(Tree));
  if (ROOMS == NULL) return -14; // Malloc failed

  MEETING_ID_LEFT = NULL; // Linked List for popped out meeting_ids

  // Initiate monitoring structure
  int i = 0;
  for (i = 0; i < meeting_room_num; i++)
  {
    ROOMS[i].count = 0;
    ROOMS[i].room = i;
    ROOMS[i].root = root;
    ROOMS[i].min = MAX_TIME;
    ROOMS[i].max = MIN_TIME;
  }

  ACTIVE_ROOMS = meeting_room_num;
  return 0;
}

/*
 *  Function: meeting_system_add
 *  ----------------------------
 *  Add a meeting to the meeting system
 * 
 *	In:
 *	start_time: Meeting start time
 *	end_time: Meeting end time
 *
 *	Out:
 *	meeting_id: meeting ID given by the system
 *	meeting_room_id: meeting room ID for the meeting
 *
 *	Return value:
 *	0 if removing meeting succeeded, a negative number representing the error otherwise
 * -11 If the meeting starting time is after the meeting ending time
 * -12 If the meeting starting time is too much in the past
 * -13 If the minimum time interval is lower than defined (e.g. less than 1 minute = 60)
 * -21 could not allocate memory (malloc failed)
 * -31 All rooms are booked - no available space
 * -32 A node should have been deleted but it doesnt exist anymore
 * -33 One of the tree input was NULL
 * -34 All rooms are booked - no available space
 * -41 max meeting_id reached
 * -51 meeting_id was inserted more than once
 * -52 meeting_id node in doesn't exist
 * -53 meeting_id tree input was NULL
 */
int meeting_system_add(time_t start_time, time_t end_time, unsigned int *meeting_id, unsigned int *meeting_room_id)
{
  static unsigned int counter = 0;
  static unsigned int meeting_id_counter = 0;
  int error_code = 0;
  struct LinkedList *meeting_id_child_nodeed_list_node = NULL;
  struct rb_node_rooms *added = NULL;

  error_code = check_time(start_time, end_time);
  if (error_code) return error_code;

  // Check if there is meeting_id that was removed in the past, if there is put it as the next meeting_id
  if (MEETING_ID_LEFT != NULL) {
    meeting_id_counter = MEETING_ID_LEFT->val;
    meeting_id_child_nodeed_list_node = MEETING_ID_LEFT;
  }
  else {
    if (meeting_id_counter > MAX_MEETING_IDS) {
      error_code = -41;
      return error_code;
    }
    meeting_id_counter += 1;
  } 

  // Find a room the put the new meeting insdie, start with the room least booked
  // (START -> counter -> num_rooms -> 0 -> counter -> END)
  // PART 1
  int i = counter;
  for (; i < ACTIVE_ROOMS; i++)
  {
    insertion_rooms(&(ROOMS[i].root), &added, start_time, end_time, &error_code);
    if (!error_code)
    {
      ROOMS[i].count += 1;
      if (ROOMS[i].min > start_time) ROOMS[i].min = start_time;
      if (ROOMS[i].max < end_time) ROOMS[i].max = end_time;
      break;
    }
    if (error_code == -21)
      return error_code; // Mem allocation error
  }
  if (error_code)
  {
    i = 0;
    // PART 2 
    for (; i < counter; i++)
    {
      insertion_rooms(&(ROOMS[i].root), &added, start_time, end_time, &error_code);
      if (!error_code)
      {
        ROOMS[i].count += 1;
        if (ROOMS[i].min > start_time) ROOMS[i].min = start_time;
        if (ROOMS[i].max < end_time) ROOMS[i].max = end_time;
        break;
      }
      if (error_code == -21)
        return error_code; // Mem allocation error
    }
  }

  if (!error_code)
  {
    *meeting_room_id = i;
    *meeting_id = meeting_id_counter;
    insertion_meeting_id(&meetings_root, added, *meeting_id, *meeting_room_id, &error_code);
    if (error_code) deletion_rooms(&(ROOMS[i - 1].root), start_time, end_time, &error_code); // error_code will now validates if we can not remove the meeting from the room tree
  }

  if (error_code == 0) counter = ((counter + 1) % ACTIVE_ROOMS); // Step counter to distribute the meetings inside the trees with logic

  if (meeting_id_child_nodeed_list_node != NULL){
    MEETING_ID_LEFT = MEETING_ID_LEFT->next;
    free(meeting_id_child_nodeed_list_node);
  }

  return error_code;
}


/*
 * Function: meeting_system_remove
 * ----------------------------
 *	Add a meeting to the meeting system
 *
 *	In:
 *	meeting_id: Meeting to delete
 *
 *	Return value:
 *	0 if removing meeting succeeded, a negative number representing the error otherwise
 *
 *  Error coodes:
 * -32 room_node in range doesn't exist - Meeting_id doesn't exist
 * -33 room tree source was NULL
 * -52 meeting_id node in range doesn't exist
 * -53 meeting_id tree source was NULL
 */
int meeting_system_remove(unsigned int meeting_id)
{
  int error = 0;
  time_t start_time;
  time_t end_time;
  struct rb_node_rooms *room_node;
  unsigned int room_id;

  // Deletes meeting id from the red-black tree that contains all the current active meetings
  deletion_meeting_id(&meetings_root, meeting_id, &error, &room_node, &room_id);
  if (error) return error;

  // Deletes meeting id from the speicifc room red-black tree that contains this meetings
  start_time = room_node->start_time;
  end_time = room_node->end_time;
  deletion_rooms(&(ROOMS[room_id].root), start_time, end_time, &error);
  if (error) return error;

  // Fix monitoring structure information - count and min / max values
  ROOMS[room_id].count -= 1;
  fix_min_max_room_tree(room_id, start_time, end_time);
  
  // Put back into the pool the removed meeting_id 
  struct LinkedList *child_nodeed_list_node;
  child_nodeed_list_node = (struct LinkedList *)malloc(sizeof(struct LinkedList));
  child_nodeed_list_node->val = meeting_id;
  child_nodeed_list_node->next = MEETING_ID_LEFT;
  MEETING_ID_LEFT = child_nodeed_list_node;

  return 0;
}


/*
 * Function: meeting_system_get_room_id
 * ----------------------------
 *	Add a meeting to the meeting system
 *
 *	In:
 *	meeting_id: Meeting to delete
 *
 *	Out:
 *	meeting_room_id: meeting room ID for the meeting
 *
 *	Return value:
 *	0 if getting meeting room succeeded, a negative number representing the error otherwise
 */
int meeting_system_get_room_id(unsigned int meeting_id, unsigned int *meeting_room_id)
{
  int error = 0;
  struct rb_node_meeting_id *stack[98], *ptr;
  int dir[98], ht;
  find_meeting_id_node(&meetings_root, &ptr, stack, dir, &ht, meeting_id, &error); // Find node
  if (error) return error;

  *meeting_room_id = ptr->room_id;
  return error;
}

/*
 * Function: meeting_system_remove
 * ----------------------------
 *	Add a meeting to the meeting system
 *
 *	In:
 *	meeting_id: Meeting to change time of
 *	start_time: Meeting desired start time
 *	end_time: Meeting desired end time
 *
 *  Out:
 *	meeting_room_id: meeting room ID for the meeting
 *
 *	Return value:
 *	0 if changing meeting time succeeded, a negative number representing the error otherwise
 * -31 There is no available spot for the new meeting
 * -32 Node in range doesn't exist
 * -33 source input was NULL
 * -34 Node in range cannot exist
 * -52 Node in range doesn't exist
 * -52 meeting_id doesnt exist in mode
 * -52 meeting_id doesnt exist in mode
 * -53 meeting root tree is NULL
 */
int meeting_system_change(unsigned int meeting_id, time_t start_time, time_t end_time, unsigned int *meeting_room_id)
{
  int error = 0;
  error = check_time(start_time, end_time);
  if (error)
    return error;
  
  struct rb_node_meeting_id *meeting_id_stack[98], *meeting_id_node;
  int dir[98], ht, i;
  unsigned int current_meeting_room_id;

  find_meeting_id_node(&meetings_root, &meeting_id_node, meeting_id_stack, dir, &ht, meeting_id, &error); // Find the meeting_id for all the info
  if (error) return error;

  struct rb_node_rooms *meeting_room_stack[98], *room_node, *optional_node;
  int room_dir[98], room_ht = 0, child_dir;

  // Extract the information from the meeting_id node
  room_node = meeting_id_node->room_node;
  current_meeting_room_id = meeting_id_node->room_id;
  time_t old_start_time = room_node->start_time;
  time_t old_end_time = room_node->end_time;

  *meeting_room_id = current_meeting_room_id;

  // Serach for new time interval
  if (old_start_time >= end_time || old_end_time <= start_time) // 0 % overlap, try to keep the same room
  { 
    find_node_rooms(&(ROOMS[current_meeting_room_id].root), &optional_node, meeting_room_stack, room_dir, &room_ht, &child_dir, start_time, end_time, &error);
    room_ht = 0; 
    if (error == -32){
      error = 0;

      deletion_rooms(&(ROOMS[current_meeting_room_id].root), old_start_time, old_end_time, &error);
      if (error) return error;
      insertion_rooms(&(ROOMS[current_meeting_room_id].root), &optional_node, start_time, end_time, &error);
      if (error) return error;

      meeting_id_node->room_node = optional_node;
      fix_min_max_room_tree(current_meeting_room_id, old_start_time, old_end_time);
      
      return 0;
    }
  }
  else if (old_start_time <= start_time && old_end_time >= end_time) // New meeting is part of the existing timeframe (it's shorter)
  {
      short_fix_min_max_room_tree(current_meeting_room_id, old_start_time, old_end_time, start_time ,end_time);
      room_node->end_time = end_time;
      room_node->start_time = start_time;
      return 0;
  }

  else if (old_end_time <= end_time || old_start_time >= start_time) // New meeting overlaps (it's longer)
  {
      if (old_start_time > start_time) // Check meeting availability before
      {
        find_node_rooms(&(ROOMS[current_meeting_room_id].root), &optional_node, meeting_room_stack, room_dir, &room_ht, &child_dir, start_time, old_start_time, &error);
        room_ht = 0;
      }
      else
        error = -32;
      if (error == -32 && old_end_time < end_time) // Check meeting availability after
        find_node_rooms(&(ROOMS[current_meeting_room_id].root), &optional_node, meeting_room_stack, room_dir, &room_ht, &child_dir, old_end_time, end_time, &error);
      if (error == -32) 
      {
        short_fix_min_max_room_tree(current_meeting_room_id, old_start_time, old_end_time, start_time ,end_time);
        room_node->end_time = end_time;
        room_node->start_time = start_time;
        return 0;
      }
  }
  
  // If here, no time slot was find in the same room - Look for other rooms.
  for (i = 0; i < ACTIVE_ROOMS; i++)
  {
    find_node_rooms(&(ROOMS[i].root), &optional_node, meeting_room_stack, room_dir, &room_ht, &child_dir, start_time, end_time, &error);
    if (error == -32)
    {
      error = 0;
      insertion_rooms(&(ROOMS[i].root), &optional_node, start_time, end_time, &error);
      if (error) return error;
      deletion_rooms(&(ROOMS[current_meeting_room_id].root), old_start_time, old_end_time, &error);
      if (error) return error;
      meeting_id_node->room_node = optional_node;

      fix_min_max_room_tree(current_meeting_room_id, old_start_time, old_end_time);
      
      if (ROOMS[i].min > start_time) ROOMS[i].min = start_time;
      if (ROOMS[i].max < end_time) ROOMS[i].max = end_time;

      *meeting_room_id = i;
      ROOMS[i].count+=1;
      ROOMS[current_meeting_room_id].count-=1;
      return 0;
      }
    }

  return -31;
}

/*
 * Function: meeting_system_suggest
 * ----------------------------
 *	Suggest a meeting start time with the same time length availibility closest to to the desired meeting time
 *	e.g. if I asked for the slot between 14:00-16:00, but there is only availibility between 15:30-17:30, the system will return 15:30
 *
 *	In:
 *	start_time: Meeting desired start time
 *	end_time: Meeting desired end time
 *
 *	Out:
 *	suggested_start_time: closest meeting time which has a room available for the same time length
 *
 *	Return value:
 *	0 if query succeeded, a negative number representing the error otherwise
 * -1 if not available spots
 */
int meeting_system_suggest(time_t start_time, time_t end_time, time_t *suggested_start_time)
{
  int error_code = 0;
  error_code = check_time(start_time, end_time);
  if (error_code)
    return error_code;

  time_t const_delta = end_time-start_time;
  time_t current_delta = 0;
  time_t max_gap = MAX_TIME;

  struct rb_node_rooms *stack[98], *ptr, *lPtr, *rPtr;
  int dir[98], ht = 0, i=0, child_dir;

  for (; i < ACTIVE_ROOMS; i++)
  {
    find_node_rooms(&(ROOMS[i].root), &ptr, stack, dir, &ht, &child_dir, start_time, end_time, &error_code);

    if (error_code == -32 || error_code == -33) // Can be placed in the exact requested time
    {
      *suggested_start_time = start_time;
      return 0;
    }
    else
    {
      rPtr = ptr;
      lPtr = ptr->threaded[0];
      while (lPtr != NULL){ // Search from center to left
        if (rPtr->start_time - lPtr->end_time >= const_delta){ 
          current_delta = rPtr->start_time-const_delta;
          if (start_time-current_delta < max_gap)
          {
            *suggested_start_time = current_delta;
            max_gap = start_time-current_delta;
          }
          break;
        }
        rPtr = lPtr;
        lPtr = lPtr->threaded[0];
      }
      if (lPtr == NULL && rPtr!=NULL){ // Last node (most-left)
        current_delta = rPtr->start_time-const_delta;
        if (start_time-current_delta < max_gap)
        {
          *suggested_start_time = current_delta;
          max_gap = start_time-current_delta;
        }
      }

      lPtr = ptr;
      rPtr = ptr->threaded[1];
      while (rPtr != NULL){ // Search from center to right
        if (rPtr->start_time - lPtr->end_time >= const_delta){
          current_delta = lPtr->end_time;
          if (current_delta-start_time < max_gap)
          {
            *suggested_start_time = current_delta;
            max_gap = current_delta-start_time;
          }  
          break;
        }
        lPtr = rPtr;
        rPtr = rPtr->threaded[1];
      }
      if (rPtr == NULL && lPtr!=NULL)// Last node (most-right)
      {
      current_delta = lPtr->end_time;
      if (current_delta-start_time < max_gap)
      {
        *suggested_start_time = current_delta;
        max_gap = current_delta-start_time;
      }
      }
    }
  }
  error_code = (max_gap == MAX_TIME)? -1: 0;
  return error_code;
}


/*
 * Function: check_meeting_system_suggest
 * ----------------------------
 *	Testing function for the following func: check_meeting_system_suggest 
*/
void check_meeting_system_suggest(){
  
  unsigned int meeting_id = 0;
  unsigned int meeting_room_id = 0;
  int error;
  time_t suggested = 0;
  time_t now, tmp;
  now = time(NULL);

  // Single room test
  error = meeting_system_init(1);
  tmp = MIN_INTERVAL;
  MIN_INTERVAL = 1;
  error = meeting_system_add(now+107, now+109, &meeting_id, &meeting_room_id);
  error = meeting_system_add(now+100, now+105, &meeting_id, &meeting_room_id);
  error = meeting_system_add(now+90, now+99, &meeting_id, &meeting_room_id);
  error = meeting_system_add(now+82, now+87, &meeting_id, &meeting_room_id);
  error = meeting_system_add(now+70, now+75, &meeting_id, &meeting_room_id);

  error = meeting_system_suggest(now+108, now+109, &suggested);
  assert(suggested == now+109);
  error = meeting_system_suggest(now+106, now+109, &suggested); 
  assert(suggested == now+109);
  error = meeting_system_suggest(now+102, now+103, &suggested);
  assert(suggested == now+99);
  error = meeting_system_suggest(now+88, now+103, &suggested);  
  assert(suggested == now+109);
  error = meeting_system_suggest(now+50, now+103, &suggested);  
  assert(suggested == now+17);

  // Multiple rooms test
  error = meeting_system_init(5);
  int i, j;
  for (j=0; j<5; j++)
  {
  for (i=0; i<5; i++) error = meeting_system_add(now+100*j+20*(i+1), now+100*j+100+20*(i+1), &meeting_id, &meeting_room_id);
  }
  error = meeting_system_suggest(now+10, now+100, &suggested);
  assert(suggested == now+10);
  error = meeting_system_suggest(now+20, now+110, &suggested);
  assert(suggested == now+10);
  error = meeting_system_suggest(now+50, now+80, &suggested); 
  assert(suggested == now+50);
  meeting_system_suggest(now+150, now+200, &suggested);
  assert(suggested == now+50);
  error = meeting_system_suggest(now+500, now+600, &suggested);  
  assert(suggested == now+520);

  MIN_INTERVAL = tmp;
}


void check_meeting_system_add(){
  time_t s, e, now, tmp;
  now = time(NULL);
  int error;
  unsigned int meeting_id = 0;
  unsigned int meeting_room_id = 0;
  error = meeting_system_init(5);
  tmp = MIN_INTERVAL;
  MIN_INTERVAL = 1;
  //1
  s = now+25;
  e = now+50;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  assert(!error);
  s+= 10;
  e-= 10;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);
  //2
  s = now+250;
  e = now+500;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);

  assert(!error);
  s-= 10;
  e+= 10;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);
  //3
  s = now+2500;
  e = now+5000;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  assert(!error);
  s-= 10;
  e-= 10;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);
  //4
  s = now+25000;
  e = now+50000;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  assert(!error);
  s-= 10;
  e-= 10;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);
  //5
  s = now+250000;
  e = now+500000;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  assert(!error);
  s+= 10;
  e+= 10;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);
  //6
  s = now+8;
  e = now+10;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  assert(!error);
  s = 2;
  e = 4;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);
  //7
  s = now+25000000;
  e = now+50000000;
  error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  assert(!error);
  s = s*2;
  e = e+2;
  error = meeting_system_change(meeting_id, s, e, &meeting_room_id);
  assert(!error);

  MIN_INTERVAL = tmp;
}


check_input_errors(){
  time_t s, e, now, tmp;
  now = time(NULL);
  int error;
  unsigned int meeting_id = 0;
  unsigned int meeting_room_id = 0;
  int i, j;
  // ERROR CODE -11

  tmp = MIN_INTERVAL;
  MIN_INTERVAL = 60;

  for (i = 1; i < 3; i++)
  {
      error = 0;
      s =  now+60*30*(i);
      e = now+61*25*(i);
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
      assert(error == -11);
  }

  // ERROR CODE -12
  for (i = 1; i < 3; i++)
  {
      error = 0;
      s =  MIN_TIME - 200*i;
      e = MIN_TIME - 50*i;
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
      assert(error == -12);
  }

  // ERROR CODE -13
  for (i = 1; i < 3; i++)
  {
      error = 0;
      s =  now+i*(i);
      e = now+i*(i);
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
      assert(error == -13);
  }

  s =  now+60*25;
  e = now+61*30;
  for (i=0; i<ACTIVE_ROOMS; i++) meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  // ERROR CODE -31
  for (i = 0; i < ACTIVE_ROOMS; i++)
  {
      error = 0;
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
      assert(error == -31);
  }

  s =  now+60*25;
  e = now+61*70;
  for (i=0; i<ACTIVE_ROOMS; i++) meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  // ERROR CODE -34
  for (i = 0; i < ACTIVE_ROOMS; i++)
  {
      error = 0;
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
      if (error != -31 || error != -34)
        assert(error == -34);
  }

  MIN_INTERVAL = tmp;
}




void random_data_pointer(){
  time_t s, e, now, tmp;
  now = time(NULL);
  int error;
  unsigned int meeting_id = 0;
  unsigned int meeting_room_id = 0;
  int i,j;

  tmp = MIN_INTERVAL;
  MIN_INTERVAL = 60;
  // Data
  for (j = 1; j < ACTIVE_ROOMS+1; j++)
  {
    for (i = 1; i < 30; i++)
    {
      error = 0;
      s = now+ (i)*(j*100);
      e = now+ (i)*(j*100)+ 1;
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
    }
  }


// Data Removal
  for (j = 0; j < ACTIVE_ROOMS; j++)
  {
    for (i = 1; i < 30; i++)
    {
      error = meeting_system_remove(meeting_id);
      meeting_id -= 1;
    }
  }

  for (j = 1; j < ACTIVE_ROOMS+1; j++)
  {
    for (i = 1; i < 2; i++)
    {
      error = 0;
      s = now+(i)*(j*100);
      e = now+(i)*(j*100)+ 1;
      error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
    }
  }

  int dpoints[] = {900, 600, 2000, 300, 800, 1400, 2600, 200, 3300, 2400, 1500, 1200, 4200};


  for (i = 0; i < 13; i++)
  {
    error = 0;
    s = now+dpoints[i];
    e = now+dpoints[i]+ 1;

    error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
    // printf("meeting_id: %d  ||  meeting_room_id: %d \n", meeting_id, meeting_room_id);
    // if (error) printf("\nERROR INSERT NODE:  %d %lld || %lld \n", error, s, e);
  }



  int remove_points[] = {meeting_id-6, meeting_id, meeting_id-1, meeting_id-2, meeting_id-3, meeting_id-4, meeting_id-12, 
                        meeting_id-11, meeting_id-5, meeting_id-7, meeting_id-8, meeting_id-9, meeting_id-10};

  for (i = 0; i < 13; i++)
  {
    error = meeting_system_remove(remove_points[i]);
    if (error) printf("\nERROR meeting_system_remove:  %d -- meeting_room_id: %d\n", error, meeting_id);
    meeting_id -= 1;
  }


int dpointsS[] = {10, 30, 200, 300, 800, 1400, 2600, 200, 3300, 2400, 1500, 1200, 4200};
int dpointsE[] = {15, 45, 500, 300, 800, 1400, 2600, 200, 3300, 2400, 1500, 1200, 4200};

for (i = 0; i < 13; i++)
  {
    error = 0;
    s = now+10;
    e = now+dpoints[i]+ 1;
    error = meeting_system_add(s, e, &meeting_id, &meeting_room_id);
  }

MIN_INTERVAL = tmp;
}



/*
 * Function: checks
 * ----------------------------
 *	Testing function
 */
void checks(void)
{

//check_meeting_system_add();
check_meeting_system_suggest();
//random_data_pointer();

}



int main(void)
{
  unsigned int meeting_id = 0;
  unsigned int meeting_room_id = 0;
  int error = 0;

  /* Get the current time */
  time_t now;
  time_t s;
  time_t e;
  now = time(NULL);
  error = meeting_system_init(20);
  if (error)
    printf("\nERROR INIT SYSTEME\n");

  printf("\n\n Start all the checks \n\n");
  checks();
  printf("\n\n Finished all the checks \n\n");

  print_all_trees_rooms();

  free_all();
  return 0;
}


void free_all()
{
  printf("\nReleasing all trees memory\n");
  delete_all_trees();
  printf("\nReleasing all rooms memory\n");
  free(ROOMS);
  printf("\nReleasing meeting id child_nodeedList\n");
  free_linked_list();
  printf("\nDone\n");
}
