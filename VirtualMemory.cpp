#include "VirtualMemory.h"
#include "PhysicalMemory.h"

typedef uint64_t physical_address;
//TODO change the checks on table size from offsetwidth to page size
struct AddressInformation
{
    physical_address address;
    bool error;
    uint64_t depth;
};
struct FarPage
{
    physical_address address;
    uint64_t virtualAddress;
    uint64_t distance;
};

inline physical_address GET_INITIAL_PAGE_SIZE ()
{
    return VIRTUAL_ADDRESS_WIDTH % OFFSET_WIDTH ? VIRTUAL_ADDRESS_WIDTH
                                                  % OFFSET_WIDTH : OFFSET_WIDTH;
}

physical_address translate_address (physical_address offset,
                                    physical_address addr, uint64_t
                                    depth)
{
    (void) depth;
    return addr * PAGE_SIZE + offset;
//    if (depth)
//        return addr * PAGE_SIZE + offset;
//    return addr * (1LL << GET_INITIAL_PAGE_SIZE ()) + offset;
}

void flushTable (uint64_t frameIndex, uint64_t depth)
{
    physical_address neighbors = depth ? PAGE_SIZE :
                                 (1LL << GET_INITIAL_PAGE_SIZE ());
    for (uint64_t i = 0; i < neighbors; ++i)
    {
        bool contained = translate_address (i, frameIndex, depth) < RAM_SIZE;
        if (contained)
        {
            word_t _word;
            PMread (translate_address (i, frameIndex, depth), &_word);
            if (_word != 0)
            {
                flushTable (_word, depth + 1);
            }
            PMwrite (translate_address (i, frameIndex, depth), 0);
        }
    }
}



struct TreePath
{
    physical_address paths[TABLES_DEPTH];
};

//TreePath get_path (uint64_t virtualAddress)
//{
//    TreePath tree_path = {};
//    virtualAddress >>= OFFSET_WIDTH;
//    for (uint64_t i = 0; i < TABLES_DEPTH - 1; virtualAddress >>= OFFSET_WIDTH, i++)
//    {
//        uint64_t _index = TABLES_DEPTH - i - 1;
//        physical_address val = virtualAddress & (PAGE_SIZE - 1);
//        tree_path.paths[_index] = val;
//    }
//    tree_path.paths[0] =
//            virtualAddress & ((1LL << GET_INITIAL_PAGE_SIZE ()) - 1);
//    return tree_path;
//}
//recursive helper function for the DFS search for unused frames
uint64_t maxUsedFrameInDFS(uint64_t CurrentDepth, word_t lastAllocatedFrame)
{
    if (CurrentDepth == TABLES_DEPTH)
        return 0;
    uint64_t maxUsedFrame;
    uint64_t currentMaxFrame = 0;
    word_t currentWord = 0;
    uint64_t temp;
    for (uint64_t i = 0; i<PAGE_SIZE;i++)
    {
        PMread(translate_address (i,lastAllocatedFrame,i),&currentWord);
        if (!currentWord)
        {
        }
        else {
            if (currentMaxFrame < currentWord) {
                currentMaxFrame = currentWord;
            }
            temp = maxUsedFrameInDFS(CurrentDepth + 1, currentWord);
            if (currentMaxFrame < temp) {
                currentMaxFrame = temp;
            }
        }
    }
    maxUsedFrame = currentMaxFrame;
    return maxUsedFrame;
}
uint64_t getCyclicalDistance(uint64_t num1, uint64_t num2)
{
    if (num1>num2)
        return getCyclicalDistance(num2,num1);
    uint64_t CyclicalDistance;
    CyclicalDistance = num2-num1;
    if(CyclicalDistance>(NUM_PAGES+num1-num2))
        CyclicalDistance = NUM_PAGES+num1-num2;
    return CyclicalDistance;
}
uint64_t cleanFrame(uint64_t frame)
{
    int Depth = 0;
    if (frame!=0)
        Depth = TABLES_DEPTH -1;
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
        PMwrite(translate_address(i,frame,Depth),0);
    return frame;
}
bool IsClearFrame(uint64_t frame)
{
    bool IsFrameClear = true;
    word_t TempWord;
    uint64_t physicalAddress;
    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        physicalAddress = translate_address(i, frame, TABLES_DEPTH - 1);
        PMread(physicalAddress, &TempWord);
        if (TempWord == 0) {}
        else {
            IsFrameClear = false;
        }
    }
    return IsFrameClear;
}
struct
FarPage getFarthestUsedPageHelper(uint64_t currentPage, uint64_t Depth,FarPage lastFarPage)
{
    FarPage tempFarPage = {0,0,0};
    FarPage MaxFarPage = {0,0,0};
    //Todo check if needs to be Table_depth-1
    if (Depth == TABLES_DEPTH) {
        tempFarPage.virtualAddress = lastFarPage.virtualAddress;
        tempFarPage.distance = getCyclicalDistance(currentPage, lastFarPage.virtualAddress);
        tempFarPage.address = lastFarPage.address;
        return tempFarPage;
    }
//    uint64_t maxUsedFrame = 0;
//    uint64_t currentMaxFrame = 0;
    word_t currentWord = 0;
//    uint64_t temp;
    for (uint64_t i=0; i<PAGE_SIZE;i++)
    {
        PMread(translate_address (i,lastFarPage.address,i),&currentWord);
        if (!currentWord)
        {
        }
        else {
            tempFarPage.address = currentWord;
            tempFarPage.virtualAddress = (lastFarPage.virtualAddress << OFFSET_WIDTH) + i;
            tempFarPage = getFarthestUsedPageHelper(currentPage,Depth+1,tempFarPage);
            if (tempFarPage.distance > MaxFarPage.distance) {
                MaxFarPage.address = tempFarPage.address;
                MaxFarPage.distance = tempFarPage.distance;
                MaxFarPage.virtualAddress = tempFarPage.virtualAddress;
            }
        }
    }
    return MaxFarPage;
}
FarPage getfarthestUsedFrame(uint64_t currentPage)
{
    FarPage currentFarPage = {0,0,0};
    return getFarthestUsedPageHelper(currentPage,(uint64_t)0,currentFarPage);
}
void ClearFatherFrame(uint64_t ChildFrame, uint64_t CheckingFrame, uint64_t Depth)
{
    if (Depth == TABLES_DEPTH)
        return;
//    uint64_t maxUsedFrame = 0;
//    uint64_t currentMaxFrame = 0;
    word_t currentWord = 0;
//    uint64_t temp;
    for (uint64_t i = 0; i<PAGE_SIZE; i++) {
        PMread(translate_address(i, CheckingFrame, Depth), &currentWord);
        if (!currentWord) {
        } else {
            if ((uint64_t)currentWord == (uint64_t)ChildFrame) {
                PMwrite(translate_address(i, CheckingFrame, Depth), 0);
                return;
            }
            else
                ClearFatherFrame(ChildFrame,currentWord , Depth + 1);
        }
    }
}
uint64_t DeallocateFrame(uint64_t frame, uint64_t frameIndex = 0, bool is_leaf = false)
{
    if (is_leaf)
    {
        PMevict(frame,frameIndex);
    }
    cleanFrame(frame);
    ClearFatherFrame(frame,0,0);
    return frame;
}
//DFS function to find unused frame if exists
//return -1 if all are taken
uint64_t getUnusedFrame()
{
    uint64_t nextUnusedFrame = maxUsedFrameInDFS(0,0) + 1;
    if (nextUnusedFrame == NUM_FRAMES)
        return -1;
    return nextUnusedFrame;
}

uint64_t HandlePageFault(uint64_t currentFrame, uint64_t currentPage)
{
    //Todo implement function to choose how to handle runing out of pages
    //Todo go over all frames with get clear Frame
//    uint64_t frame;
    for (uint64_t i = 1; i < NUM_FRAMES; i++) {
        if (i == currentFrame) {}
        else {
            if (IsClearFrame(i)) {
                DeallocateFrame(i);
                return i;
            }
        }
    }
    FarPage page = getfarthestUsedFrame(currentPage);
    DeallocateFrame(page.address,page.virtualAddress,true);
    return page.address;
}

uint64_t AllocateNewFrame(physical_address FrameLocation, uint64_t virtualAddress,uint64_t Depth=0)
{
    auto unusedPage = (word_t)getUnusedFrame();
    uint64_t currentFrame = FrameLocation >> OFFSET_WIDTH;//?
    if (unusedPage == -1)
        unusedPage = (word_t)HandlePageFault(currentFrame,virtualAddress >>
        OFFSET_WIDTH);
    if(Depth == TABLES_DEPTH)
    {
        PMrestore(unusedPage,virtualAddress>>OFFSET_WIDTH);
    }
    PMwrite(FrameLocation,unusedPage);
    return unusedPage;
}
AddressInformation search_for (TreePath offsets, uint64_t virtualAddress,bool ReadOperation = true)
{
    word_t _word = 0;
    AddressInformation result = {0, false,0};
    for (uint64_t i = 0; i < TABLES_DEPTH; i++)
    {
        result.depth = i;
        physical_address _address = translate_address (offsets.paths[i], result
                .address, i);
        if (_address >= RAM_SIZE)
        {
            result.address = _address;
            result.error = true;
            return result;
        }

        // Read the address again
//        _address = translate_address (offsets.paths[i], result.address, i);
        PMread (_address, &_word);
        result.depth = i;
        if (!_word)
        {
            if(ReadOperation) {
//                _address = translate_address(offsets.paths[i], result
//                        .address, i);
                result.error = true;
                result.address = _address;
                return result;
            }
            else
                result.address = AllocateNewFrame(_address,virtualAddress,i+1);
//                _address = translate_address (offsets.paths[i], result
//                        .address, i);
//            result.next_address = -1;
//            result.address = _address;
//            return result;
        }
        else
        {
            result.address = _word;
        }
    }
//    result.address = _word;
    return result;
}

//uint64_t writeNode (physical_address addr, uint64_t frame)
//{
//    PMwrite (addr, (word_t) frame);
//    return 1;
//}

physical_address getAddress(uint64_t virtualAddress, bool ReadOperation = false)
{
//    word_t FrameToAllocate;
    uint64_t VirtualoffsetAddress = virtualAddress & (PAGE_SIZE-1);
//    physical_address physicalOffset;
    TreePath OffsetsTree = {};
    for (uint64_t currentDepth = TABLES_DEPTH; currentDepth > 0; currentDepth--)
        OffsetsTree.paths[TABLES_DEPTH - currentDepth] = (virtualAddress >> (((currentDepth) * OFFSET_WIDTH))&
                                           (( 1LL<< OFFSET_WIDTH) - 1));
    AddressInformation addressToWriteTo = search_for(OffsetsTree, virtualAddress, ReadOperation);
    if (addressToWriteTo.error)
        return RAM_SIZE+1;
    return translate_address(VirtualoffsetAddress,addressToWriteTo.address,addressToWriteTo.depth);
}
int VMread (uint64_t virtualAddress, word_t *value) {
    if ( (virtualAddress >= ( VIRTUAL_MEMORY_SIZE << WORD_WIDTH)) )
      return 0;
    uint64_t _address = getAddress(virtualAddress);
    if (_address == RAM_SIZE+1) {
        return 0;
    }
    if(_address >= RAM_SIZE || virtualAddress >= VIRTUAL_MEMORY_SIZE){
      return 0;
    }
    PMread(_address, value);
    return 1;
}



int VMwrite(uint64_t virtualAddress, word_t value) {
    if (virtualAddress > VIRTUAL_MEMORY_SIZE)
        return 0;
    physical_address addressToWriteTo = getAddress(virtualAddress, false);
    if (addressToWriteTo == RAM_SIZE+1)
        return 0;
    PMwrite(addressToWriteTo,value);
    return 1;
}
void VMinitialize ()
{
    cleanFrame(0);
//    flushTable (0, 0);
//    for (uint64_t i = 1; i < RAM_SIZE; i++)
//    {
//        flushTable (i, 1);
//    }
}