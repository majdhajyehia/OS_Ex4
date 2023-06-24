#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "math.h"

typedef uint64_t physical_address;

struct AddressInformation
{
    physical_address address;
    physical_address next_address;
    uint64_t depth;
};

inline physical_address GET_INITIAL_PAGE_SIZE ()
{
    return VIRTUAL_ADDRESS_WIDTH % OFFSET_WIDTH ? VIRTUAL_ADDRESS_WIDTH
                                                  % OFFSET_WIDTH : OFFSET_WIDTH;
}

physical_address translate_address (physical_address offset,
                                    physical_address addr, int depth)
{
    if (depth)
        return addr * PAGE_SIZE + offset;
    return addr * (1LL << GET_INITIAL_PAGE_SIZE ()) + offset;
}

void flushTable (uint64_t frameIndex, int depth)
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

void VMinitialize ()
{
    flushTable (0, 0);
    for (int i = 1; i < RAM_SIZE; i++)
    {
        flushTable (i, 1);
    }
}

struct TreePath
{
    physical_address paths[TABLES_DEPTH + 1];
};

TreePath get_path (uint64_t virtualAddress)
{
    TreePath tree_path = {};
    virtualAddress >>= OFFSET_WIDTH;
    for (int i = 0; i < TABLES_DEPTH - 1; virtualAddress >>= OFFSET_WIDTH, i++)
    {
        int _index = TABLES_DEPTH - i - 1;
        physical_address val = virtualAddress & (PAGE_SIZE - 1);
        tree_path.paths[_index] = val;
    }
    tree_path.paths[0] =
            virtualAddress & ((1LL << GET_INITIAL_PAGE_SIZE ()) - 1);
    return tree_path;
}
//recursive helper function for the DFS search for unused frames
int maxUsedFrameInDFS(int CurrentDepth, word_t lastAllocatedFrame)
{
    if (CurrentDepth == TABLES_DEPTH)
        return 0;
    int maxUsedFrame = 0;
    int currentMaxFrame = 0;
    word_t currentWord = 0;
    int temp;
    for (int i; i<OFFSET_WIDTH;i++)
        PMread(translate_address (i,lastAllocatedFrame,i),&currentWord);
    if (!currentWord) {
    } else
    if (currentMaxFrame<currentWord)
        currentMaxFrame = currentWord;
    temp = maxUsedFrameInDFS(CurrentDepth + 1, currentWord);
    if (currentMaxFrame<temp)
        currentMaxFrame = temp;
    maxUsedFrame = currentMaxFrame;
    return maxUsedFrame;
}
//DFS function to find unused frame if exists
//return -1 if all are taken
int getUnusedFrame()
{
    int nextUnusedFrame = maxUsedFrameInDFS(0,0) + 1;
    if (nextUnusedFrame == NUM_FRAMES)
        return -1;
    return nextUnusedFrame;
}

int getCyclicalDistance()
{

}
word_t HandlePageFault()
{
    //Todo implement function to choose how to handle runing out of pages
}
word_t AllocateNewPage(physical_address PageLocation)
{
    word_t unusedPage = getUnusedFrame();
    if (unusedPage == -1)
        //Todo implement handlePagefault
        unusedPage = HandlePageFault();
    //Todo clean unused page
    PMwrite(PageLocation,unusedPage);
    return unusedPage;
}
AddressInformation search_for (TreePath offsets)
{
    word_t _word = 0;
    AddressInformation result = {0, 0};
    for (int i = 0; i < TABLES_DEPTH; i++)
    {
        result.depth = i;
        physical_address _address = translate_address (offsets.paths[i], result
                .address, i);
        if (_address >= RAM_SIZE)
        {
            result.address = _address;
            result.next_address = -1;
            return result;
        }

        // Read the address again
        _address = translate_address (offsets.paths[i], result
                .address, i);
        PMread (_address, &_word);
        result.depth = i;
        if (!_word)
        {
            //TODO add pageFault in AllocateNew Page
            result.address = AllocateNewPage(_address);
//            _address = translate_address (offsets.paths[i], result
//                    .address, i);
//            result.next_address = -1;
//            result.address = _address;
//            return result;
        }
        else
        {
            result.address = _word;
        }
    }
    result.address = _word;
    return result;
}

int writeNode (physical_address addr, uint64_t frame)
{
    PMwrite (addr, (word_t) frame);
    return 1;
}


int VMread (uint64_t virtualAddress, word_t *value)
{
    //TODO: handle eadge case of reading something that doesn't exist
    // TODO: Implement the getAddress
    AddressInformation _address = getAddress (virtualAddress);
    int depth = (int) _address.depth;
    physical_address translated_address = translate_address (
            virtualAddress
            & (PAGE_SIZE - 1), _address.address, depth);
    if ((virtualAddress >= (VIRTUAL_MEMORY_SIZE << WORD_WIDTH)))
        return 0;
    bool translated_ended = translated_address >= RAM_SIZE;
    bool virtual_ended = virtualAddress >= VIRTUAL_MEMORY_SIZE;
    bool should_halt = virtual_ended || translated_ended;
    if (should_halt)
    {
        return 0;
    }
    PMread (translated_address, (word_t *) value);
    return 1;
}
physical_address getPageAddress(uint_fast64_t virtualAddress)
{
    word_t FrameToAllocate;
    int VirtualoffsetAddress = virtualAddress & (PAGE_SIZE-1);
    //Todo handle case in which page isn't allocated yet
    physical_address physicalOffset;
    TreePath OffsetsTree;
    for (int currentDepth = TABLES_DEPTH - 1; currentDepth >= 0; currentDepth--)
        OffsetsTree.paths[currentDepth] = (virtualAddress >> (((currentDepth) * OFFSET_WIDTH))&
                                           (( 1LL<< OFFSET_WIDTH) - 1));

    //Todo add handling int search_for for the case of non existing page
    //Todo clean frame before allocating it to a new location
    AddressInformation addressToWriteTo = search_for(OffsetsTree);
    while (addressToWriteTo.next_address == -1)
        //Todo: allocate empty frame and search again
        addressToWriteTo = search_for(OffsetsTree);
    return translate_address(VirtualoffsetAddress,addressToWriteTo.address,addressToWriteTo.depth);
}



int VMwrite(uint64_t virtualAddress, word_t value) {
    int offsetAddress = virtualAddress & (OFFSET_WIDTH-1);
    int CurrentDepth = 0;
    word_t* NextAddress;
    PMread(translate_address(0,0,0),NextAddress);
    if (NextAddress)
        //Todo finish the function
        return 0;
}
