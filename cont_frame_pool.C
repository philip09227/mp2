/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/


ContFramePool* ContFramePool::node_head;
ContFramePool* ContFramePool::list;


ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    
    // bitmap can fit in one single frame
    assert(_n_frames <= FRAME_SIZE *8);

    base_frame_no = _base_frame_no; // the id of the first frame, in kernel first frame is 512nd frame
    n_frames = _n_frames; // how many frames in this pool,  2^9 frames
    nFreeFrames = _n_frames; 
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;
    

    // bitmap == address
    // if info_fram =0 means management information store in this frame
    // we use first frame in this poll to store information
    // and set the bitmap(address) to the address of first frame
    // aka. " Frame Number * Frame Size" frame number== base_fram
    // this time bitmap should be at 2MB  200000(hex) 
    if (info_frame_no == 0){
	    bitmap = (unsigned char *) ( base_frame_no * FRAME_SIZE);

    }else{
	    bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE); 
	    // if not in the first frame set address to the current frame
	    // which contain the information 
	                                                          
    }
    
    assert((n_frames %8) == 0);
    
    

    // status of fram  00 means free
    // 01 means head
    // 10 meand in accessible
    // 11 means allocated 
    for (int i =0; i*4< n_frames; i++){
	    bitmap[i] = 0x00;
    }
    
    // set first frame in fram pool to allocated 
    if(info_frame_no==0){
	    bitmap[0] = 0x40;
	    nFreeFrames--;
    }

    // if list has not beem declared
    if ( ContFramePool::node_head == NULL)
    {
	    ContFramePool::node_head = this;
	    ContFramePool::list = this;
    }
    else
    {
	    ContFramePool::list->next = this;
	    ContFramePool::list = this;
    }
    next = NULL;



    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
   
    assert(nFreeFrames > _n_frames );
    
    // first number of frame pool
    unsigned long frame_no = base_frame_no;
    unsigned int i =0;
    unsigned int frames_needed = _n_frames;
    unsigned int count =0;
    unsigned int same_frame = 0;
    unsigned int find = 0;
    unsigned char checker = 0xc0;
    // find the first frames which has consecutive number of frames we can use
    while (( i < n_frames/4) && (count < frames_needed)){
	    for ( int j =0 ; j < 4 && (count < frames_needed); j++){
		    // bit manipulation if i and checker = 0 means it's free 
		    if ( (bitmap[i] & checker) == 0x0){
			    if ( same_frame == 1){
				    count ++; 
                            }
			    else{
				    same_frame =1;
				    count++;
				    frame_no += i*4+j;
			    }
		    }else{
                     // this fram is occupied need to reset search
		     // if previous found fram availabe need to reset
		     // if not go to next fram directly
		     if(same_frame ==1){
			     // means previous search found fram available
			     //reset
			     count = 0;
		             same_frame = 0;
			     frame_no = base_frame_no;
		    }
		    checker = checker >>2;
		    //if found enough frames break
		    if ( count == frames_needed){
			    find = 1; // set the flag if not find return 0
		            break;
		    }
	    }
            // do the early break same as outter loop
	    if (count == frames_needed){
		    find =1;
		    break;
	    }
            checker = 0xc0;   
            i++;	    
    } 
    if ( find == 0){
	    return 0;
    }
    }
    // find enough frames we need set the status first to 11 and rest to 01
    // now the fram_no is the first frame 
    // in order to set the checker to correct number need to know the difference between 
    // current frame and base frame 
    unsigned char checker_head = 0x40 >>(((frame_no - base_frame_no)%4)*2);
    unsigned char checker_reset = 0xc0 >> (((frame_no - base_frame_no)%4)*2);
    unsigned char checker_allocated = 0xc0 >> (((frame_no - base_frame_no)%4)*2);
    unsigned int bitmap_index = ((frame_no - base_frame_no)/4); 
    // set the head to 01
    bitmap[bitmap_index] =(( bitmap[bitmap_index] & (~checker_reset)) | checker_head); 
    checker_reset = checker_reset >> 2;
    i = 1;
    while ( i< _n_frames)
    {
	    while((i < _n_frames) && (checker_reset != 0x0))
	    {
                    bitmap[bitmap_index] = (bitmap[bitmap_index] & (~checker_reset)) | checker_allocated;
		    checker_reset = checker_reset >> 2;
		    i++;
	    }
	    checker_reset = 0xc0;
	    bitmap_index++;
    }
    nFreeFrames -= _n_frames;
    return (frame_no);
 
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{  
    unsigned long start = _base_frame_no;
    unsigned long end = _base_frame_no + _n_frames;
    unsigned int bitmap_index;
    unsigned char checker;
    unsigned char checker_reset;
    for ( start; start < end; start++)
    {
	    bitmap_index = ( (start - base_frame_no)/4);
	    checker_reset = 0xc0 << ((start%4)*2);
	    checker = 0x80 >> ((start%4)*2);
	    // whater the value is reset it 
	    bitmap[bitmap_index] = bitmap[bitmap_index] & ( ~checker_reset);
	    // set the first two bit to 10 which means inaccessable
	    bitmap[bitmap_index] = bitmap[bitmap_index] | checker;
    }

}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // need to check this frame pool possess the frame we want to release
    ContFramePool* current = ContFramePool::node_head;
    while (( current->base_frame_no > _first_frame_no) || ( _first_frame_no >= current->base_frame_no + current->n_frames))
    {
	    current = current->next;
    }

    // check if frame is head of sequence 
    unsigned int bitmap_index = (_first_frame_no - current->base_frame_no)/4;
    unsigned char checker_head = 0x80 >>(((_first_frame_no - current->base_frame_no)%4)*2);
    unsigned char checker_reset = 0xc0 >> (((_first_frame_no - current->base_frame_no)%4*2));
    unsigned int i;
    if(((current->bitmap[bitmap_index] ^ checker_head) & checker_reset) == checker_reset)
    {
          // reset head to free frame
	  current->bitmap[bitmap_index] = current->bitmap[bitmap_index]&(~checker_reset); 
    }

    // set the rest consective frame to free frame 
    for ( i = _first_frame_no; i < current->base_frame_no + current->n_frames; i++)
    {
        int index = ( i - current->base_frame_no)/4;
        checker_reset = checker_reset >> (i - current->base_frame_no)%4;
        if ((current->bitmap[index] & checker_reset)==0)
	{
	     break;
	}
	if(( current->bitmap[index] & checker_reset ) ==0)
	{
            break;
	}
	current->bitmap[index] = current->bitmap[index] & (~ checker_reset);
	
     
    }



}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
	// 4K = 4096
	return (_n_frames*2/ 8*4096) + ( (_n_frames*2) % (8*4096) > 0 ? 1 : 0);
}
