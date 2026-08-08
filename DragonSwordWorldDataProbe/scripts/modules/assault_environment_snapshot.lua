local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local out=mod_dir.."\\runtime\\reports\\assault-environment-current.tsv"

local classes={"DETWeatherControlActor","DWeatherControlActor","DClimateControlActor","DTimeManager","DDayNightManager"}
local keys={"Weather","WeatherID","CurrentWeather","Climate","ClimateID","CurrentClimate","DaySwitchID","DaySwitch","Time","WorldTime","Hour","Day","DayID","Season"}

local function valid(o)if o==nil then return false end local ok,v=pcall(function()return o:IsValid()end);return ok and v==true end
local function scalar(v)if v==nil then return "" end if type(v)=="string" or type(v)=="number" or type(v)=="boolean" then return tostring(v)end local ok,s=pcall(function()return v:ToString()end);if ok then return tostring(s)end return tostring(v)end
local function field(o,n)local ok,v=pcall(function()return o[n]end);if ok and v~=nil then return v end local ok2,v2=pcall(function()return o:GetPropertyValue(n)end);if ok2 then return v2 end return nil end
local function full(o)local s="";pcall(function()s=o:GetFullName()end);return tostring(s or "")end
local function clean(v)return tostring(v or ""):gsub("\t"," "):gsub("[\r\n]"," ")end

local function run(ctx)
 local rows={}
 for _,cn in ipairs(classes)do
  local o=nil;pcall(function()o=FindFirstOf(cn)end)
  if valid(o) then
   for _,k in ipairs(keys)do
    local v=field(o,k)
    if v~=nil then rows[#rows+1]={Class=cn,Object=full(o),Property=k,Value=scalar(v)} end
   end
  end
 end
 local f=io.open(out,"w")
 if f then
  f:write("UTC\tClass\tObject\tProperty\tValue\n")
  local utc=os.date("!%Y-%m-%dT%H:%M:%SZ")
  for _,r in ipairs(rows)do f:write(utc,"\t",clean(r.Class),"\t",clean(r.Object),"\t",clean(r.Property),"\t",clean(r.Value),"\n")end
  f:close()
 end
 ctx.log("ASSAULT_ENVIRONMENT_SNAPSHOT",{rows=#rows,unknown_function_invocations=0})
 return {status="ok",value_type="row_count",value=tostring(#rows),fingerprint="assault_environment:"..tostring(#rows)}
end
return {id="assault_environment_snapshot",enabled=true,description="Read-only weather/climate/day/time snapshot for trigger-condition correlation.",steps={{kind="custom",name="assault_environment_snapshot",class="<environment classes>",role="assault_condition_discovery",purpose="read_only_environment_snapshot",cadence=1,run=run}}}
