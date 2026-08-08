local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_dir=mod_dir.."\\runtime\\reports"

local current_path=report_dir.."\\assault-quest-trigger-current.tsv"
local history_path=report_dir.."\\assault-quest-trigger-history.tsv"
local function_path=report_dir.."\\assault-quest-system-functions.tsv"

local classes={
 "DETTask_TagQuestStepLoopInStandAlone",
 "DLayerEvent_GoalQuest",
}

local systems={
 "DClientQuestSystem",
 "DsPGQuestSubsystem",
 "DsPGQuestMainSubsystem",
}

local interesting={
 "QuestID","QuestId","MissionID","MissionId","UnexpectedMissionID",
 "PlaceID","KindID","ISRegister","IsRegister","bIsRegister","IsRegistered",
 "Active","IsActive","bActive","Enabled","IsEnabled",
 "State","QuestState","MissionState","Step","StepID","CurrentStep",
 "Clear","IsClear","Completed","IsCompleted","Complete","IsComplete"
}

local previous={}

local function unwrap(v)
 if v==nil then return nil end
 local ok,t=pcall(function()return v:type()end)
 if ok and (t=="RemoteUnrealParam" or t=="LocalUnrealParam") then
  local ok2,g=pcall(function()return v:get()end)
  if ok2 then return g end
 end
 return v
end
local function valid(o)
 o=unwrap(o); if o==nil then return false end
 local ok,v=pcall(function()return o:IsValid()end); return ok and v==true
end
local function scalar(v)
 v=unwrap(v); if v==nil then return "" end
 if type(v)=="string" or type(v)=="number" or type(v)=="boolean" then return tostring(v) end
 local ok,s=pcall(function()return v:ToString()end);if ok and s~=nil then return tostring(s) end
 return tostring(v)
end
local function field(o,n)
 o=unwrap(o);if o==nil then return nil end
 local ok,v=pcall(function()return o[n]end);if ok and v~=nil then return unwrap(v) end
 local ok2,v2=pcall(function()return o:GetPropertyValue(n)end);if ok2 then return unwrap(v2) end
 return nil
end
local function full(o)
 o=unwrap(o);if o==nil then return "" end
 local s="";pcall(function()s=o:GetFullName()end);return tostring(s or "")
end
local function fname(o)
 local s="";pcall(function()s=o:GetFName():ToString()end);return tostring(s or "")
end
local function cname(o)
 local s="";pcall(function()s=o:GetClass():GetFullName()end);return tostring(s or "")
end
local function off(o)
 local v=-1;pcall(function()v=o:GetOffset_Internal()end);return tonumber(v) or -1
end
local function clean(v)return tostring(v or ""):gsub("\t"," "):gsub("[\r\n]"," ")end

local headers={"UTC","Class","Object","Index","QuestID","MissionID","UnexpectedMissionID","PlaceID","KindID","ISRegister","Active","Enabled","State","Step","Clear","Completed","Fingerprint"}

local function write(path,rows,append)
 local exists=false
 if append then local x=io.open(path,"r");if x then exists=true;x:close()end end
 local f=io.open(path,append and "a" or "w");if not f then return false end
 if not exists then f:write(table.concat(headers,"\t"),"\n")end
 for _,r in ipairs(rows)do
  local vals={};for _,h in ipairs(headers)do vals[#vals+1]=clean(r[h])end
  f:write(table.concat(vals,"\t"),"\n")
 end
 f:close();return true
end

local function first_value(o,names)
 for _,n in ipairs(names)do
  local v=field(o,n)
  if v~=nil then return scalar(v) end
 end
 return ""
end

local function snapshot_class(name)
 local rows={}
 if type(FindAllOf)~="function" then return rows end
 local ok,objects=pcall(function()return FindAllOf(name)end)
 if not ok or type(objects)~="table" then return rows end
 local limit=math.min(#objects,256)
 for i=1,limit do
  local o=objects[i]
  if valid(o) then
   local row={
    UTC=os.date("!%Y-%m-%dT%H:%M:%SZ"),Class=name,Object=full(o),Index=i,
    QuestID=first_value(o,{"QuestID","QuestId"}),
    MissionID=first_value(o,{"MissionID","MissionId"}),
    UnexpectedMissionID=first_value(o,{"UnexpectedMissionID","UnexpectedMissionId"}),
    PlaceID=first_value(o,{"PlaceID","PlaceId"}),
    KindID=first_value(o,{"KindID","KindId"}),
    ISRegister=first_value(o,{"ISRegister","IsRegister","bIsRegister","IsRegistered"}),
    Active=first_value(o,{"Active","IsActive","bActive"}),
    Enabled=first_value(o,{"Enabled","IsEnabled","bEnabled"}),
    State=first_value(o,{"State","QuestState","MissionState"}),
    Step=first_value(o,{"Step","StepID","CurrentStep"}),
    Clear=first_value(o,{"Clear","IsClear"}),
    Completed=first_value(o,{"Completed","IsCompleted","Complete","IsComplete"}),
   }
   row.Fingerprint=table.concat({row.QuestID,row.MissionID,row.UnexpectedMissionID,row.PlaceID,row.KindID,row.ISRegister,row.Active,row.Enabled,row.State,row.Step,row.Clear,row.Completed},"|")
   rows[#rows+1]=row
  end
 end
 return rows
end

local function inventory_system_functions()
 local rows={}
 for _,name in ipairs(systems)do
  local obj=nil;pcall(function()obj=FindFirstOf(name)end)
  if valid(obj) then
   local cls=nil;pcall(function()cls=obj:GetClass()end)
   if cls then
    pcall(function()
     cls:ForEachFunction(function(fn)
      local n=string.lower(fname(fn))
      if n:find("quest",1,true) or n:find("register",1,true) or n:find("clear",1,true) or n:find("mission",1,true) or n:find("state",1,true) or n:find("active",1,true) then
       local props={}
       pcall(function()
        fn:ForEachProperty(function(p)props[#props+1]={n=fname(p),t=cname(p),o=off(p)}end)
       end)
       table.sort(props,function(a,b)return a.o<b.o end)
       local ps={};local ret=""
       for _,p in ipairs(props)do
        if string.lower(p.n)=="returnvalue" then ret=p.t else ps[#ps+1]=p.n..":"..p.t.."@"..tostring(p.o) end
       end
       rows[#rows+1]={System=name,FunctionName=fname(fn),FunctionPath=full(fn),Parameters=table.concat(ps,";"),ReturnType=ret}
      end
     end)
    end)
   end
  end
 end
 local f=io.open(function_path,"w")
 if f then
  f:write("System\tFunctionName\tFunctionPath\tParameters\tReturnType\n")
  for _,r in ipairs(rows)do f:write(clean(r.System),"\t",clean(r.FunctionName),"\t",clean(r.FunctionPath),"\t",clean(r.Parameters),"\t",clean(r.ReturnType),"\n")end
  f:close()
 end
 return #rows
end

local function run(ctx)
 local rows={}
 for _,name in ipairs(classes)do
  local part=snapshot_class(name)
  for _,r in ipairs(part)do rows[#rows+1]=r end
 end
 table.sort(rows,function(a,b)return a.Object<b.Object end)
 write(current_path,rows,false)

 local changed={}
 local now={}
 for _,r in ipairs(rows)do
  local key=r.Class.."|"..r.Object
  now[key]=r
  local old=previous[key]
  if old==nil or old.Fingerprint~=r.Fingerprint then changed[#changed+1]=r end
 end
 for key,old in pairs(previous)do
  if now[key]==nil then
   local r={};for k,v in pairs(old)do r[k]=v end
   r.UTC=os.date("!%Y-%m-%dT%H:%M:%SZ");r.State="DISAPPEARED"
   changed[#changed+1]=r
  end
 end
 if #changed>0 then write(history_path,changed,true) end
 previous=now

 local fn_count=inventory_system_functions()
 ctx.log("ASSAULT_QUEST_TRIGGER_CORRELATION",{objects=#rows,changed=#changed,quest_system_functions=fn_count,unknown_function_invocations=0})
 return {status="ok",value_type="counts",value="objects="..#rows..";changed="..#changed..";functions="..fn_count,fingerprint="quest_trigger:"..#rows..":"..#changed}
end

return {id="assault_quest_trigger_correlation",enabled=true,description="Read-only exact Quest Trigger/Step and Quest-system state correlation.",steps={{kind="custom",name="assault_quest_trigger_snapshot",class="<quest trigger classes>",role="assault_availability_discovery",purpose="read_only_state_correlation",cadence=1,run=run}}}
