// See the file "COPYING" in the main distribution directory for copyright.

// ZAM: Zeek Abstract Machine compiler.

#pragma once

#include "zeek/Event.h"
#include "zeek/script_opt/ReachingDefs.h"
#include "zeek/script_opt/UseDefs.h"
#include "zeek/script_opt/ZAM/ZBody.h"

namespace zeek::detail {

class NameExpr;
class ConstExpr;
class FieldExpr;
class ListExpr;
class EventHandler;

class Stmt;
class SwitchStmt;
class CatchReturnStmt;

class ProfileFunc;

typedef ZInstI* InstLabel;

// Class representing a single compiled statement.  (This is different from,
// but related to, the ZAM instruction(s) generated for that compilation.)
// Designed to be fully opaque, but also effective without requiring pointer
// management.
class ZAMStmt {
protected:
	friend class ZAMCompiler;

	ZAMStmt(int _stmt_num)	{ stmt_num = _stmt_num; }

	int stmt_num;
};

// Class that holds values that only have meaning to the ZAM compiler,
// but that needs to be held (opaquely, via a pointer) by external
// objects.
class OpaqueVals;

class ZAMCompiler {
public:
	ZAMCompiler(ScriptFunc* f, std::shared_ptr<ProfileFunc> pf,
	            ScopePtr scope, StmtPtr body, std::shared_ptr<UseDefs> ud,
	            std::shared_ptr<Reducer> rd);

	~ZAMCompiler();

	StmtPtr CompileBody();

	void Dump();

	// Public so that GenInst flavors can get to it.
	int FrameSlot(const NameExpr* id)
		{ return FrameSlot(id->AsNameExpr()->Id()); }
	int Frame1Slot(const NameExpr* id, ZOp op)
		{ return Frame1Slot(id->AsNameExpr()->Id(), op); }

private:
	void Init();

	void SetCurrStmt(const Stmt* stmt)	{ curr_stmt = stmt; }

#include "zeek/ZAM-MethodDecl.h"

	const ZAMStmt ConstructTable(const NameExpr* n, const Expr* e);
	const ZAMStmt ConstructSet(const NameExpr* n, const Expr* e);
	const ZAMStmt ConstructRecord(const NameExpr* n, const Expr* e);
	const ZAMStmt ConstructVector(const NameExpr* n, const Expr* e);

	const ZAMStmt ArithCoerce(const NameExpr* n, const Expr* e);
	const ZAMStmt RecordCoerce(const NameExpr* n, const Expr* e);
	const ZAMStmt TableCoerce(const NameExpr* n, const Expr* e);
	const ZAMStmt VectorCoerce(const NameExpr* n, const Expr* e);

	const ZAMStmt Is(const NameExpr* n, const Expr* e);

	const ZAMStmt IfElse(const Expr* e, const Stmt* s1, const Stmt* s2);
	const ZAMStmt While(const Stmt* cond_stmt, const Expr* cond,
	                    const Stmt* body);

	// Second argument is which instruction slot holds the branch target.
	const ZAMStmt GenCond(const Expr* e, int& branch_v);
	const ZAMStmt Loop(const Stmt* body);

	const ZAMStmt When(Expr* cond, const Stmt* body, const Expr* timeout,
	                   const Stmt* timeout_body, bool is_return);

	const ZAMStmt Switch(const SwitchStmt* sw);

	const ZAMStmt For(const ForStmt* f);

	const ZAMStmt Call(const ExprStmt* e);
	const ZAMStmt AssignToCall(const ExprStmt* e);
	const ZAMStmt DoCall(const CallExpr* c, const NameExpr* n);

	const ZAMStmt AssignVecElems(const Expr* e);
	const ZAMStmt AssignTableElem(const Expr* e);

	const ZAMStmt AppendToField(const NameExpr* n1, const NameExpr* n2,
	                            const ConstExpr* c, int offset);

	const ZAMStmt InitRecord(ID* id, RecordType* rt);
	const ZAMStmt InitVector(ID* id, VectorType* vt);
	const ZAMStmt InitTable(ID* id, TableType* tt, Attributes* attrs);

	const ZAMStmt Return(const ReturnStmt* r);
	const ZAMStmt CatchReturn(const CatchReturnStmt* cr);

	const ZAMStmt CompileSchedule(const NameExpr* n,
					const ConstExpr* c, int is_interval,
					EventHandler* h, const ListExpr* l);

	const ZAMStmt CompileEvent(EventHandler* h, const ListExpr* l);

	const ZAMStmt ValueSwitch(const SwitchStmt* sw, const NameExpr* v,
					const ConstExpr* c);
	const ZAMStmt TypeSwitch(const SwitchStmt* sw, const NameExpr* v,
					const ConstExpr* c);

	void PushNexts()		{ PushGoTos(nexts); }
	void PushBreaks()		{ PushGoTos(breaks); }
	void PushFallThroughs()		{ PushGoTos(fallthroughs); }
	void PushCatchReturns()		{ PushGoTos(catches); }

	void ResolveNexts(const InstLabel l)
		{ ResolveGoTos(nexts, l); }
	void ResolveBreaks(const InstLabel l)
		{ ResolveGoTos(breaks, l); }
	void ResolveFallThroughs(const InstLabel l)
		{ ResolveGoTos(fallthroughs, l); }
	void ResolveCatchReturns(const InstLabel l)
		{ ResolveGoTos(catches, l); }


	const ZAMStmt LoopOverTable(const ForStmt* f, const NameExpr* val);
	const ZAMStmt LoopOverVector(const ForStmt* f, const NameExpr* val);
	const ZAMStmt LoopOverString(const ForStmt* f, const NameExpr* val);

	const ZAMStmt FinishLoop(const ZAMStmt iter_head, ZInstI iter_stmt,
	                         const Stmt* body, int info_slot);

	const ZAMStmt Next()
		{ return GenGoTo(nexts.back()); }
	const ZAMStmt Break()
		{ return GenGoTo(breaks.back()); }
	const ZAMStmt FallThrough()
		{ return GenGoTo(fallthroughs.back()); }
	const ZAMStmt CatchReturn()
		{ return GenGoTo(catches.back()); }


	const ZAMStmt CompileInExpr(const NameExpr* n1, const NameExpr* n2,
	                            const NameExpr* n3)
		{ return CompileInExpr(n1, n2, nullptr, n3, nullptr); }

	const ZAMStmt CompileInExpr(const NameExpr* n1, const NameExpr* n2,
	                            const ConstExpr* c)
		{ return CompileInExpr(n1, n2, nullptr, nullptr, c); }

	const ZAMStmt CompileInExpr(const NameExpr* n1, const ConstExpr* c,
	                            const NameExpr* n3)
		{ return CompileInExpr(n1, nullptr, c, n3, nullptr); }

	// In the following, one of n2 or c2 (likewise, n3/c3) will be nil.
	const ZAMStmt CompileInExpr(const NameExpr* n1, const NameExpr* n2,
	                            const ConstExpr* c2, const NameExpr* n3,
	                            const ConstExpr* c3);

	const ZAMStmt CompileInExpr(const NameExpr* n1, const ListExpr* l,
	                            const NameExpr* n2)
		{ return CompileInExpr(n1, l, n2, nullptr); }

	const ZAMStmt CompileInExpr(const NameExpr* n, const ListExpr* l,
	                            const ConstExpr* c)
		{ return CompileInExpr(n, l, nullptr, c); }

	const ZAMStmt CompileInExpr(const NameExpr* n1, const ListExpr* l,
	                            const NameExpr* n2, const ConstExpr* c);


	const ZAMStmt CompileIndex(const NameExpr* n1, const NameExpr* n2,
	                           const ListExpr* l);
	const ZAMStmt CompileIndex(const NameExpr* n1, const ConstExpr* c,
	                           const ListExpr* l);
	const ZAMStmt CompileIndex(const NameExpr* n1, int n2_slot,
	                           TypePtr n2_type, const ListExpr* l);


#include "zeek/script_opt/ZAM/BuiltIn.h"

	// A bit weird, but handy for switch statements used in built-in
	// operations: returns a bit mask of which of the arguments in the
	// given list correspond to constants, with the high-ordered bit
	// being the first argument (argument "0" in the list) and the
	// low-ordered bit being the last.  Second parameter is the number
	// of arguments that should be present.
	bro_uint_t ConstArgsMask(const ExprPList& args, int nargs) const;


	// Returns a handle to state associated with building
	// up a list of values.
	OpaqueVals* BuildVals(const ListExprPtr&);

	// "stride" is how many slots each element of l will consume.
	ZInstAux* InternalBuildVals(const ListExpr* l, int stride = 1);

	// Returns how many values were added.
	int InternalAddVal(ZInstAux* zi, int i, Expr* e);


	typedef std::vector<ZAMStmt> GoToSet;
	typedef std::vector<GoToSet> GoToSets;

	void PushGoTos(GoToSets& gotos);
	void ResolveGoTos(GoToSets& gotos, const InstLabel l);

	ZAMStmt GenGoTo(GoToSet& v);
	ZAMStmt GoToStub();
	ZAMStmt GoTo(const InstLabel l);
	InstLabel GoToTarget(const ZAMStmt s);
	InstLabel GoToTargetBeyond(const ZAMStmt s);
	ZAMStmt PrevStmt(const ZAMStmt s);
	void SetV(ZAMStmt s, const InstLabel l, int v)
		{
		if ( v == 1 )
			SetV1(s, l);
		else if ( v == 2 )
			SetV2(s, l);
		else if ( v == 3 )
			SetV3(s, l);
		else
			SetV4(s, l);
		}

	void SetTarget(ZInstI* inst, const InstLabel l, int slot);
	void SetV1(ZAMStmt s, const InstLabel l);
	void SetV2(ZAMStmt s, const InstLabel l);
	void SetV3(ZAMStmt s, const InstLabel l);
	void SetV4(ZAMStmt s, const InstLabel l);
	void SetGoTo(ZAMStmt s, const InstLabel targ)
		{ SetV1(s, targ); }


	const ZAMStmt StartingBlock();
	const ZAMStmt FinishBlock(const ZAMStmt start);

	bool NullStmtOK() const;

	const ZAMStmt AddInst(const ZInstI& inst);

	const ZAMStmt EmptyStmt();
	const ZAMStmt ErrorStmt();
	const ZAMStmt LastInst();

	// Returns the last (interpreter) statement in the body.
	const Stmt* LastStmt(const Stmt* s) const;

	// Returns the most recent added instruction *other* than those
	// added for bookkeeping (like dirtying globals);
	ZInstI* TopMainInst()	{ return insts1[top_main_inst]; }

	bool IsUnused(const ID* id, const Stmt* where) const;

	// Called to synchronize any globals that have been modified
	// prior to switching to execution out of the current function
	// body (for a call or a return).  The argument is a statement
	// or expression, used to find reaching-defs.  A nil value
	// corresponds to "running off the end" (no explicit return).
	void SyncGlobals(const Obj* o);

	void LoadParam(ID* id);
	const ZAMStmt LoadGlobal(ID* id);

	int AddToFrame(ID*);

	int FrameSlot(const ID* id);
	int FrameSlotIfName(const Expr* e)
		{
		auto n = e->Tag() == EXPR_NAME ? e->AsNameExpr() : nullptr;
		return n ? FrameSlot(n->Id()) : 0;
		}

	int ConvertToInt(const Expr* e)
		{
		if ( e->Tag() == EXPR_NAME )
			return FrameSlot(e->AsNameExpr()->Id());
		else
			return e->AsConstExpr()->Value()->AsInt();
		}

	int ConvertToCount(const Expr* e)
		{
		if ( e->Tag() == EXPR_NAME )
			return FrameSlot(e->AsNameExpr()->Id());
		else
			return e->AsConstExpr()->Value()->AsCount();
		}

	int Frame1Slot(const ID* id, ZOp op)
		{ return Frame1Slot(id, op1_flavor[op]); }
	int Frame1Slot(const NameExpr* n, ZAMOp1Flavor fl)
		{ return Frame1Slot(n->Id(), fl); }
	int Frame1Slot(const ID* id, ZAMOp1Flavor fl);

	// The slot without doing any global-related checking.
	int RawSlot(const NameExpr* n)	{ return RawSlot(n->Id()); }
	int RawSlot(const ID* id);

	bool HasFrameSlot(const ID* id) const;

	int NewSlot(const TypePtr& t)
		{ return NewSlot(ZVal::IsManagedType(t)); }
	int NewSlot(bool is_managed);

	int TempForConst(const ConstExpr* c);

	void SyncGlobals(std::unordered_set<ID*>& g, const Obj* o);


// #include "zeek/ZOpt.h"

	// The first of these is used as we compile down to ZInstI's.
	// The second is the final intermediary code.  They're separate
	// to make it easy to remove dead code.
	std::vector<ZInstI*> insts1;
	std::vector<ZInstI*> insts2;

	// Used as a placeholder when we have to generate a GoTo target
	// beyond the end of what we've compiled so far.
	ZInstI* pending_inst = nullptr;

	// Indices of break/next/fallthrough/catch-return goto's, so they
	// can be patched up post-facto.  These are vectors-of-vectors
	// so that nesting works properly.
	GoToSets breaks;
	GoToSets nexts;
	GoToSets fallthroughs;
	GoToSets catches;

	// The following tracks return variables for catch-returns.
	// Can be nil if the usage doesn't include using the return value
	// (and/or no return value generated).
	std::vector<const NameExpr*> retvars;

	ScriptFunc* func;
	std::shared_ptr<ProfileFunc> pf;
	std::shared_ptr<Scope> scope;
	StmtPtr body;
	std::shared_ptr<UseDefs> ud;
	std::shared_ptr<Reducer> reducer;

	// Maps identifiers to their (unique) frame location.
	std::unordered_map<const ID*, int> frame_layout1;

	// Inverse mapping, used for tracking frame usage (and for dumping
	// statements).
	FrameMap frame_denizens;

	// The same, but for remapping identifiers to shared frame slots.
	FrameReMap shared_frame_denizens;

	// The same, but renumbered to take into account removal of
	// dead statements.
	FrameReMap shared_frame_denizens_final;

	// Maps frame1 slots to frame2 slots.  A value < 0 means the
	// variable doesn't exist in frame2 - it's an error to encounter
	// one of these when remapping instructions!
	std::vector<int> frame1_to_frame2;

	// A type for mapping an instruction to a set of locals associated
	// with it.
	typedef std::unordered_map<const ZInstI*, std::unordered_set<ID*>>
		AssociatedLocals;

	// Maps (live) instructions to which frame denizens begin their
	// lifetime via an initialization at that instruction, if any ...
	// (it can be more than one local due to extending lifetimes to
	// span loop bodies)
	AssociatedLocals inst_beginnings;

	// ... and which frame denizens had their last usage at the
	// given instruction.  (These are inst1 instructions, prior to
	// removing dead instructions, compressing the frames, etc.)
	AssociatedLocals inst_endings;

	// A type for inverse mappings.
	typedef std::unordered_map<int, const ZInstI*> AssociatedInsts;

	// Inverse mappings: for a given frame denizen's slot, where its
	// lifetime begins and ends.
	AssociatedInsts denizen_beginning;
	AssociatedInsts denizen_ending;

	// In the following, member variables ending in 'I' are intermediary
	// values that get finalized when constructing the corresponding
	// ZBody.
	std::vector<GlobalInfo> globalsI;
	std::unordered_map<const ID*, int> global_id_to_info;	// inverse

	// Which globals are potentially ever modified.
	std::unordered_set<const ID*> modified_globals;

	// The following are used for switch statements, mapping the
	// switch value (which can be any atomic type) to a branch target.
	// We have vectors of them because functions can contain multiple
	// switches.
	template<class T> using CaseMapI = std::map<T, InstLabel>;
	template<class T> using CaseMapsI = std::vector<CaseMapI<T>>;

	CaseMapsI<bro_int_t> int_casesI;
	CaseMapsI<bro_uint_t> uint_casesI;
	CaseMapsI<double> double_casesI;

	// Note, we use this not only for strings but for addresses
	// and prefixes.
	CaseMapsI<std::string> str_casesI;

	void DumpIntCases(int i) const;
	void DumpUIntCases(int i) const;
	void DumpDoubleCases(int i) const;
	void DumpStrCases(int i) const;

	std::vector<int> managed_slotsI;

	int frame_sizeI;

	// Number of iteration loops in the body.  Used to size the
	// vector of IterInfo's used for recursive functions.
	int num_iters = 0;

	bool non_recursive = false;

	// Most recent instruction, other than for housekeeping.
	int top_main_inst;

	// Used for communication between Frame1Slot and a subsequent
	// AddInst.  If >= 0, then upon adding the next instruction,
	// it should be followed by Dirty-Global for the given slot.
	int mark_dirty = -1;
};

} // namespace zeek::detail

// Invokes after compiling all of the function bodies.
class FuncInfo;
extern void finalize_functions(const std::vector<FuncInfo*>& funcs);