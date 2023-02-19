--[[
    Vanilla portal mod to connect portals together throughout gameplay

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]

local WARD_PREFAB = VUtils.String.GetStableHashCode("guard_stone")

local RPC_vha = function(peer, cmd, args)

    if not peer.admin then return end
    
    print("Got command " .. cmd)
    
    if cmd == "claim" then
        if #args == 1 then
            local radius = tonumber(args[1]) or 32
            
            local zdos = ZDOManager.GetZDOs(peer.pos, radius, WARD_PREFAB)
            
            for i=1, #zdos do
                Views.Ward.new(zdos[i]).creator = peer.name
            end
            
        else
            peer.Message("missing radius", MsgType.console)
        end
    elseif cmd == "op" then
        if #args == 1 then
            local p = NetManager.GetPeer(args[1])
            if p then p.admin = not p.admin end
        else
            peer.Message("missing player to op", MsgType.console)
        end
    end
end

Valhalla.OnEvent("Join", function(peer)

    print("Registering vha")

    peer.value:Register(
        MethodSig.new("vha", DataType.string, DataType.strings),
        RPC_vha
    )

    print("Registered vha")

end)

Valhalla.OnEvent("Enable", function()
    print(this.name .. " enabled")
end)
