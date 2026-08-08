local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_dir=mod_dir.."\\runtime\\reports"
local done=false

local function valid(o)
 if o==nil then return false end
 local ok,v=pcall(function() return o:IsValid() end);return ok and v==true
end
local function clean(v)return tostring(v or ""):gsub("\t"," "):gsub("[\r\n]"," ")end
local function fname(o)local v="";pcall(function()v=o:GetFName():ToString()end);return tostring(v or "")end
local function full(o)local v="";pcall(function()v=o:GetFullName()end);return tostring(v or "")end
local function cname(o)local v="";pcall(function()v=o:GetClass():GetFullName()end);return tostring(v or "")end
local function off(o)local v=-1;pcall(function()v=o:GetOffset_Internal()end);return tonumber(v) or -1 end
local function write(path,rows)
 local f=io.open(path,"w");if not f then return false end
 f:write("Owner\tFunctionName\tFunctionPath\tParameters\tReturnType\n")
 for _,r in ipairs(rows)do f:write(clean(r.Owner),"\t",clean(r.FunctionName),"\t",clean(r.FunctionPath),"\t",clean(r.Parameters),"\t",clean(r.ReturnType),"\n")end
 f:close();return true
end
local function inventory(owner_label,obj,rows)
 if not valid(obj) then return 0 end
 local cls=nil;pcall(function()cls=obj:GetClass()end);if cls==nil then return 0 end
 local count=0
 pcall(function()
  cls:ForEachFunction(function(fn)
   count=count+1
   local props={}
   pcall(function()
    fn:ForEachProperty(function(p)props[#props+1]={n=fname(p),t=cname(p),o=off(p)}end)
   end)
   table.sort(props,function(a,b)return a.o<b.o end)
   local ps={};local ret=""
   for _,p in ipairs(props)do
    if string.lower(p.n)=="returnvalue" then ret=p.t else ps[#ps+1]=p.n..":"..p.t.."@"..tostring(p.o) end
   end
   rows[#rows+1]={Owner=owner_label,FunctionName=fname(fn),FunctionPath=full(fn),Parameters=table.concat(ps,";"),ReturnType=ret}
  end)
 end)
 return count
end
local function run(ctx)
 if done then return {status="complete",value_type="scan_state",value="already_completed",fingerprint="treasure_owner_inventory:done"} end
 local rows={};local det=StaticFindObject("/Script/DS.Default__DETUtil")
 local detn=inventory("DETUtil",det,rows)
 local prop=nil
 local notes={}
 local function try(label,fn)
  local ok,v=pcall(fn); notes[#notes+1]=label.."="..tostring(ok and valid(v))
  if ok and valid(v) and not valid(prop) then prop=v end
 end
 try("FindFirstOf_DPropDataTable",function()return FindFirstOf("DPropDataTable")end)
 try("CDO_DPropDataTable",function()return StaticFindObject("/Script/DS.Default__DPropDataTable")end)
 for _,mn in ipairs({"GameDBTableManager","DGameDBTableManager"})do
  local ok,m=pcall(function()return FindFirstOf(mn)end)
  notes[#notes+1]="manager_"..mn.."="..tostring(ok and valid(m))
  if ok and valid(m) then
   local pok,pv=pcall(function()return m.DPropDataTable end)
   notes[#notes+1]="manager_"..mn.."_DPropDataTable="..tostring(pok and valid(pv))
   if pok and valid(pv) and not valid(prop) then prop=pv end
  end
 end
 local propn=inventory("DPropDataTable",prop,rows)
 local ef=io.open(report_dir.."\\treasure-runtime-owner-evidence.tsv","w")
 if ef then
  ef:write("Key\tValue\n")
  ef:write("DPropDataTableValid\t",tostring(valid(prop)),"\n")
  ef:write("LookupNotes\t",clean(table.concat(notes,";")),"\n")
  ef:close()
 end
 write(report_dir.."\\treasure-runtime-owner-functions.tsv",rows)
 ctx.log("TREASURE_OWNER_FUNCTION_INVENTORY",{detutil_functions=detn,prop_table_functions=propn,unknown_functions_invoked=0})
 done=true
 return {status="ok",value_type="counts",value="det="..detn..";prop="..propn,fingerprint="treasure_owner_inventory:"..detn..":"..propn}
end
return {id="treasure_runtime_discovery",enabled=true,description="Read-only DETUtil + DPropDataTable function inventory; invokes no discovered function.",steps={{kind="custom",name="treasure_owner_inventory",class="<DETUtil,DPropDataTable>",role="treasure_function_discovery",purpose="read_only_signature_inventory",cadence=1,once_per_session=true,run=run}}}
