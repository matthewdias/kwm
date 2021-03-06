#include "node.h"
#include "container.h"
#include "tree.h"
#include "space.h"
#include "window.h"

extern kwm_screen KWMScreen;
extern kwm_tiling KWMTiling;
extern kwm_focus KWMFocus;

tree_node *CreateRootNode()
{
    tree_node Clear = {0};
    tree_node *RootNode = (tree_node*) malloc(sizeof(tree_node));
    *RootNode = Clear;

    RootNode->WindowID = -1;
    RootNode->Type = NodeTypeTree;
    RootNode->Parent = NULL;
    RootNode->LeftChild = NULL;
    RootNode->RightChild = NULL;
    RootNode->SplitRatio = KWMScreen.SplitRatio;
    RootNode->SplitMode = SPLIT_OPTIMAL;

    return RootNode;
}

link_node *CreateLinkNode()
{
    link_node Clear = {0};
    link_node *Link = (link_node*) malloc(sizeof(link_node));
    *Link = Clear;

    Link->WindowID = -1;
    Link->Prev = NULL;
    Link->Next = NULL;

    return Link;
}

tree_node *CreateLeafNode(screen_info *Screen, tree_node *Parent, int WindowID, int ContainerType)
{
    Assert(Parent)

    tree_node Clear = {0};
    tree_node *Leaf = (tree_node*) malloc(sizeof(tree_node));
    *Leaf = Clear;

    Leaf->Parent = Parent;
    Leaf->WindowID = WindowID;
    Leaf->Type = NodeTypeTree;

    CreateNodeContainer(Screen, Leaf, ContainerType);

    Leaf->LeftChild = NULL;
    Leaf->RightChild = NULL;

    return Leaf;
}

void CreateLeafNodePair(screen_info *Screen, tree_node *Parent, int FirstWindowID, int SecondWindowID, split_type SplitMode)
{
    Assert(Parent)

    Parent->WindowID = -1;
    Parent->SplitMode = SplitMode;
    Parent->SplitRatio = KWMScreen.SplitRatio;

    node_type ParentType = Parent->Type;
    link_node *ParentList = Parent->List;
    Parent->Type = NodeTypeTree;
    Parent->List = NULL;

    int LeftWindowID = KWMTiling.SpawnAsLeftChild ? SecondWindowID : FirstWindowID;
    int RightWindowID = KWMTiling.SpawnAsLeftChild ? FirstWindowID : SecondWindowID;

    if(SplitMode == SPLIT_VERTICAL)
    {
        Parent->LeftChild = CreateLeafNode(Screen, Parent, LeftWindowID, 1);
        Parent->RightChild = CreateLeafNode(Screen, Parent, RightWindowID, 2);

        tree_node *Node = KWMTiling.SpawnAsLeftChild ?  Parent->RightChild : Parent->LeftChild;
        Node->Type = ParentType;
        Node->List = ParentList;
        ResizeLinkNodeContainers(Node);
    }
    else if(SplitMode == SPLIT_HORIZONTAL)
    {
        Parent->LeftChild = CreateLeafNode(Screen, Parent, LeftWindowID, 3);
        Parent->RightChild = CreateLeafNode(Screen, Parent, RightWindowID, 4);

        tree_node *Node = KWMTiling.SpawnAsLeftChild ?  Parent->RightChild : Parent->LeftChild;
        Node->Type = ParentType;
        Node->List = ParentList;
        ResizeLinkNodeContainers(Node);
    }
    else
    {
        Parent->Parent = NULL;
        Parent->LeftChild = NULL;
        Parent->RightChild = NULL;
        Parent = NULL;
    }
}

void CreatePseudoNode()
{
    screen_info *Screen = KWMScreen.Current;
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    window_info *Window = KWMFocus.Window;
    if(!Screen || !Space || !Window)
        return;

    tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, Window->WID);
    if(Node)
    {
        split_type SplitMode = KWMScreen.SplitMode == SPLIT_OPTIMAL ? GetOptimalSplitMode(Node) : KWMScreen.SplitMode;
        CreateLeafNodePair(Screen, Node, Node->WindowID, -1, SplitMode);
        ApplyTreeNodeContainer(Node);
    }
}

void RemovePseudoNode()
{
    screen_info *Screen = KWMScreen.Current;
    space_info *Space = GetActiveSpaceOfScreen(Screen);
    window_info *Window = KWMFocus.Window;
    if(!Screen || !Space || !Window)
        return;

    tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, Window->WID);
    if(Node && Node->Parent)
    {
        tree_node *Parent = Node->Parent;
        tree_node *PseudoNode = IsLeftChild(Node) ? Parent->RightChild : Parent->LeftChild;
        if(!PseudoNode || !IsLeafNode(PseudoNode) || PseudoNode->WindowID != -1)
            return;

        Parent->WindowID = Node->WindowID;
        Parent->LeftChild = NULL;
        Parent->RightChild = NULL;
        free(Node);
        free(PseudoNode);
        ApplyTreeNodeContainer(Parent);
    }
}

bool IsLeafNode(tree_node *Node)
{
    return Node->LeftChild == NULL && Node->RightChild == NULL ? true : false;
}

bool IsPseudoNode(tree_node *Node)
{
    return Node && Node->WindowID == -1 && IsLeafNode(Node);
}

bool IsLeftChild(tree_node *Node)
{
    if(Node && IsLeafNode(Node))
    {
        tree_node *Parent = Node->Parent;
        return Parent->LeftChild == Node;
    }

    return false;
}

bool IsRightChild(tree_node *Node)
{
    if(Node && IsLeafNode(Node))
    {
        tree_node *Parent = Node->Parent;
        return Parent->RightChild == Node;
    }

    return false;
}

void ToggleNodeSplitMode(screen_info *Screen, tree_node *Node)
{
    if(!Node || IsLeafNode(Node))
        return;

    Node->SplitMode = Node->SplitMode == SPLIT_VERTICAL ? SPLIT_HORIZONTAL : SPLIT_VERTICAL;
    CreateNodeContainers(Screen, Node, false);
    ApplyTreeNodeContainer(Node);
}

void ToggleTypeOfFocusedNode()
{
    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, KWMFocus.Window->WID);
    if(TreeNode && TreeNode != Space->RootNode)
        TreeNode->Type = TreeNode->Type == NodeTypeTree ? NodeTypeLink : NodeTypeTree;
}

void ChangeTypeOfFocusedNode(node_type Type)
{
    Assert(KWMScreen.Current)
    Assert(KWMFocus.Window)

    space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
    tree_node *TreeNode = GetTreeNodeFromWindowIDOrLinkNode(Space->RootNode, KWMFocus.Window->WID);
    if(TreeNode && TreeNode != Space->RootNode)
        TreeNode->Type = Type;
}

void SwapNodeWindowIDs(tree_node *A, tree_node *B)
{
    if(A && B)
    {
        DEBUG("SwapNodeWindowIDs() " << A->WindowID << " with " << B->WindowID)
        int TempWindowID = A->WindowID;
        A->WindowID = B->WindowID;
        B->WindowID = TempWindowID;

        node_type TempNodeType = A->Type;
        A->Type = B->Type;
        B->Type = TempNodeType;

        link_node *TempLinkList = A->List;
        A->List = B->List;
        B->List = TempLinkList;

        ResizeLinkNodeContainers(A);
        ResizeLinkNodeContainers(B);
        ApplyTreeNodeContainer(A);
        ApplyTreeNodeContainer(B);
    }
}

void SwapNodeWindowIDs(link_node *A, link_node *B)
{
    if(A && B)
    {
        DEBUG("SwapNodeWindowIDs() " << A->WindowID << " with " << B->WindowID)
        int TempWindowID = A->WindowID;
        A->WindowID = B->WindowID;
        B->WindowID = TempWindowID;
        ResizeWindowToContainerSize(A);
        ResizeWindowToContainerSize(B);
    }
}

split_type GetOptimalSplitMode(tree_node *Node)
{
    return (Node->Container.Width / Node->Container.Height) >= KWMTiling.OptimalRatio ? SPLIT_VERTICAL : SPLIT_HORIZONTAL;
}

void ResizeWindowToContainerSize(tree_node *Node)
{
    window_info *Window = GetWindowByID(Node->WindowID);

    if(Window)
    {
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
        {
            SetWindowDimensions(WindowRef, Window,
                        Node->Container.X, Node->Container.Y,
                        Node->Container.Width, Node->Container.Height);

            if(WindowsAreEqual(Window, KWMFocus.Window))
                KWMFocus.Cache = *Window;
        }
    }
}

void ResizeWindowToContainerSize(link_node *Link)
{
    window_info *Window = GetWindowByID(Link->WindowID);

    if(Window)
    {
        AXUIElementRef WindowRef;
        if(GetWindowRef(Window, &WindowRef))
        {
            SetWindowDimensions(WindowRef, Window,
                        Link->Container.X, Link->Container.Y,
                        Link->Container.Width, Link->Container.Height);

            if(WindowsAreEqual(Window, KWMFocus.Window))
                KWMFocus.Cache = *Window;
        }
    }
}

void ResizeWindowToContainerSize(window_info *Window)
{
    Assert(Window)
    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *Node = GetTreeNodeFromWindowID(Space->RootNode, Window->WID);
        if(Node)
            ResizeWindowToContainerSize(Node);

        if(!Node)
        {
            link_node *Link = GetLinkNodeFromWindowID(Space->RootNode, Window->WID);
            if(Link)
                ResizeWindowToContainerSize(Link);
        }
    }
}

void ResizeWindowToContainerSize()
{
    if(KWMFocus.Window)
        ResizeWindowToContainerSize(KWMFocus.Window);
}

void ModifyContainerSplitRatio(double Offset)
{
    if(DoesSpaceExistInMapOfScreen(KWMScreen.Current))
    {
        space_info *Space = GetActiveSpaceOfScreen(KWMScreen.Current);
        tree_node *Root = Space->RootNode;
        if(IsLeafNode(Root) || Root->WindowID != -1)
            return;

        tree_node *Node = GetTreeNodeFromWindowIDOrLinkNode(Root, KWMFocus.Window->WID);
        if(Node && Node->Parent)
        {
            if(Node->Parent->SplitRatio + Offset > 0.0 &&
               Node->Parent->SplitRatio + Offset < 1.0)
            {
                Node->Parent->SplitRatio += Offset;
                ResizeNodeContainer(KWMScreen.Current, Node->Parent);
                ApplyTreeNodeContainer(Node->Parent);
            }
        }
    }
}
