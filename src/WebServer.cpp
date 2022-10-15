#include <Arduino.h>
#include <WiFi.h>

namespace ocs
{

    class WebAdmin
    {

    public:
        void loop(bool enable)
        {
            WiFiClient client = this->server.available(); // Listen for incoming clients

            if (enable)
            {
                if (client)
                { // If a new client connects,
                    this->currentTime = millis();
                    this->previousTime = this->currentTime;
                    Serial.println("New Client."); // print a message out in the serial port
                    String currentLine = "";       // make a String to hold incoming data from the client
                   
                    while (client.connected() && this->currentTime - this->previousTime <= this->timeoutTime)
                    { // loop while the client's connected
                        this->currentTime = millis();
                        if (client.available())
                        {                           // if there's bytes to read from the client,
                            char c = client.read(); // read a byte, then
                            Serial.write(c);        // print it out the serial monitor
                            this->header += c;
                            if (c == '\n')
                            { // if the byte is a newline character
                                // if the current line is blank, you got two newline characters in a row.
                                // that's the end of the client HTTP request, so send a response:
                                if (currentLine.length() == 0)
                                {

                                    client.println(this->getHandler(&currentLine));
                                    // The HTTP response ends with another blank line
                                    client.println();
                                    // Break out of the while loop
                                    break;
                                }
                                else
                                { // if you got a newline, then clear currentLine
                                    currentLine = "";
                                }
                            }
                            else if (c != '\r')
                            {                     // if you got anything else but a carriage return character,
                                currentLine += c; // add it to the end of the currentLine
                            }
                        }
                    }
                    // Clear the header variable
                    this->header = "";
                    // Close the connection
                    client.stop();
                    Serial.println("Client disconnected.");
                    Serial.println("");
                }
            }
        }

    private:
        WiFiServer server(80);
        // Current time
        unsigned long currentTime = millis();
        // Previous time
        unsigned long previousTime = 0;
        // Define timeout time in milliseconds (example: 2000ms = 2s)
        const long timeoutTime = 2000;
        // Variable to store the HTTP request
        String header;

        String header(String status, String content_type)
        {
            String r = "HTTP/1.1 " + status + " OK\n";
            r = r + "Content-type:" + content_type + "\n";
            r = r + R"(X-Powered-By: ESP32
                Connection: close
                )";
            return r;
        }

        String getHandler(String *currentLine)
        {
            String r = "";

            if (currentLine.endsWith("GET / HTTP"))
            {
                r = this->header("200", "text/html") + this->responseRoot();
            }
            else if (currentLine.endsWith("GET /build/bundle.js"))
            {
                r = this->header("200", "application/javascript") + this->responsebundleJS();
            }
            else if (currentLine.endsWith("GET /build/bundle.css"))
            {
                r = this->header("200", "text/css") + this->responseCSS();
            }
            else if (currentLine.endsWith("GET /getsettings HTTP"))
            {
                r = this->header("200", "application/json") + this->responsegetSettings();
            }
            else
            {
                r = this->header("404", "text/html", "<h1>Page not found</h1><p>Error 404</p>");
            }
            return r;
        }

        String responseRoot()
        {
            return R"(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Open Community Safety</title>
    <link rel="stylesheet" href="build/bundle.css" />
    <script defer src="build/bundle.js"></script>
  </head>
  <body></body>
</html>)";
        }

        String responsebundleJS()
        {
            return R"(var app=function(){"use strict";function t(){}function e(t){return t()}function n(){return Object.create(null)}function o(t){t.forEach(e)}function s(t){return"function"==typeof t}function i(t,e){return t!=t?e==e:t!==e||t&&"object"==typeof t||"function"==typeof t}function l(t,e){t.appendChild(e)}function a(t,e,n){t.insertBefore(e,n||null)}function u(t){t.parentNode.removeChild(t)}function r(t,e){for(let n=0;n<t.length;n+=1)t[n]&&t[n].d(e)}function c(t){return document.createElement(t)}function d(t){return document.createTextNode(t)}function f(){return d(" ")}function p(t,e,n,o){return t.addEventListener(e,n,o),()=>t.removeEventListener(e,n,o)}function h(t,e,n){null==n?t.removeAttribute(e):t.getAttribute(e)!==n&&t.setAttribute(e,n)}function g(t,e){t.value=null==e?"":e}let v;function m(t){v=t}function b(t){(function(){if(!v)throw new Error("Function called outside component initialization");return v})().$$.on_mount.push(t)}const $=[],y=[],w=[],C=[],k=Promise.resolve();let _=!1;function A(t){w.push(t)}const E=new Set;let I=0;function S(){const t=v;do{for(;I<$.length;){const t=$[I];I++,m(t),H(t.$$)}for(m(null),$.length=0,I=0;y.length;)y.pop()();for(let t=0;t<w.length;t+=1){const e=w[t];E.has(e)||(E.add(e),e())}w.length=0}while($.length);for(;C.length;)C.pop()();_=!1,E.clear(),m(t)}function H(t){if(null!==t.fragment){t.update(),o(t.before_update);const e=t.dirty;t.dirty=[-1],t.fragment&&t.fragment.p(t.ctx,e),t.after_update.forEach(A)}}const P=new Set;function L(t,e){-1===t.$$.dirty[0]&&($.push(t),_||(_=!0,k.then(S)),t.$$.dirty.fill(0)),t.$$.dirty[e/31|0]|=1<<e%31}function G(i,l,a,r,c,d,f,p=[-1]){const h=v;m(i);const g=i.$$={fragment:null,ctx:null,props:d,update:t,not_equal:c,bound:n(),on_mount:[],on_destroy:[],on_disconnect:[],before_update:[],after_update:[],context:new Map(l.context||(h?h.$$.context:[])),callbacks:n(),dirty:p,skip_bound:!1,root:l.target||h.$$.root};f&&f(g.root);let b=!1;if(g.ctx=a?a(i,l.props||{},((t,e,...n)=>{const o=n.length?n[0]:e;return g.ctx&&c(g.ctx[t],g.ctx[t]=o)&&(!g.skip_bound&&g.bound[t]&&g.bound[t](o),b&&L(i,t)),e})):[],g.update(),b=!0,o(g.before_update),g.fragment=!!r&&r(g.ctx),l.target){if(l.hydrate){const t=function(t){return Array.from(t.childNodes)}(l.target);g.fragment&&g.fragment.l(t),t.forEach(u)}else g.fragment&&g.fragment.c();l.intro&&((x=i.$$.fragment)&&x.i&&(P.delete(x),x.i($))),function(t,n,i,l){const{fragment:a,on_mount:u,on_destroy:r,after_update:c}=t.$$;a&&a.m(n,i),l||A((()=>{const n=u.map(e).filter(s);r?r.push(...n):o(n),t.$$.on_mount=[]})),c.forEach(A)}(i,l.target,l.anchor,l.customElement),S()}var x,$;m(h)}function N(t,e,n){const o=t.slice();return o[11]=e[n].wifi,o[13]=n,o}function O(t,e,n){const o=t.slice();return o[11]=e[n].wifi,o[13]=n,o}function j(t){let e,n,o,s,i,r,v;return{c(){e=c("label"),n=d("SSID "),o=d(t[13]),s=f(),i=c("input"),h(e,"for","fname"),h(i,"type","text"),h(i,"class","svelte-10a4hhp")},m(u,c){a(u,e,c),l(e,n),l(e,o),a(u,s,c),a(u,i,c),g(i,t[0].ssid[1].ssid),r||(v=p(i,"input",t[6]),r=!0)},p(t,e){1&e&&i.value!==t[0].ssid[1].ssid&&g(i,t[0].ssid[1].ssid)},d(t){t&&u(e),t&&u(s),t&&u(i),r=!1,v()}}}function M(t){let e,n,o,s,i,r,v;return{c(){e=c("label"),n=d("Password "),o=d(t[13]),s=f(),i=c("input"),h(e,"for","lname"),h(i,"type","password"),h(i,"class","svelte-10a4hhp")},m(u,c){a(u,e,c),l(e,n),l(e,o),a(u,s,c),a(u,i,c),g(i,t[0].ssid[0].pwd),r||(v=p(i,"input",t[7]),r=!0)},p(t,e){1&e&&i.value!==t[0].ssid[0].pwd&&g(i,t[0].ssid[0].pwd)},d(t){t&&u(e),t&&u(s),t&&u(i),r=!1,v()}}}function T(e){let n,s,i,d,v,m,b,x,$,y,w,C,k,_,A,E,I,S,H,P,L,G,T,F,W,Y,q,z,B,R,U,J,K,Q,V,X,Z,tt,et,nt,ot,st,it,lt,at,ut,rt,ct,dt,ft,pt,ht,gt,vt,mt,bt,xt,$t,yt,wt,Ct,kt,_t=e[0].ssid,At=[];for(let t=0;t<_t.length;t+=1)At[t]=j(O(e,_t,t));let Et=e[0].ssid,It=[];for(let t=0;t<Et.length;t+=1)It[t]=M(N(e,Et,t));return{c(){n=c("div"),s=c("h1"),s.textContent="OPEN COMMUNITY SAFETY",i=f(),d=c("div"),v=c("button"),v.textContent="Get settings",m=f(),b=c("button"),b.textContent="Save settings",x=f(),$=c("h2"),$.textContent="Settings",y=f(),w=c("div"),C=c("label"),C.textContent="Device ID",k=f(),_=c("input"),A=f(),E=c("div"),I=c("label"),I.textContent="Websocket Host",S=f(),H=c("input"),P=f(),L=c("div"),G=c("label"),G.textContent="Physical button label",T=f(),F=c("input"),W=f(),Y=c("br"),q=f(),z=c("div"),B=c("h3"),B.textContent="WIFI",R=f(),U=c("div"),J=c("div");for(let t=0;t<At.length;t+=1)At[t].c();K=f(),Q=c("div");for(let t=0;t<It.length;t+=1)It[t].c();var t,e,o,l;V=f(),X=c("br"),Z=f(),tt=c("div"),et=c("h3"),et.textContent="Geolocation",nt=f(),ot=c("div"),st=c("div"),it=c("label"),it.textContent="Latitude",lt=f(),at=c("input"),ut=f(),rt=c("div"),ct=c("label"),ct.textContent="Longitude",dt=f(),ft=c("input"),pt=f(),ht=c("div"),gt=c("button"),gt.textContent="Get Geolocation",vt=f(),mt=c("br"),bt=f(),xt=c("div"),$t=c("h3"),$t.textContent="SSL Certificate",yt=f(),wt=c("textarea"),t=s,e="color",null===(o="darkcyan")?t.style.removeProperty(e):t.style.setProperty(e,o,l?"important":""),h(v,"class","button button1 svelte-10a4hhp"),h(b,"class","button button1 svelte-10a4hhp"),h(d,"class","button_ali svelte-10a4hhp"),h(C,"for","fname"),h(_,"type","text"),h(_,"name","deviceId"),h(_,"maxlength","40"),h(_,"class","svelte-10a4hhp"),h(I,"for","fname"),h(H,"type","text"),h(H,"name","websockethost"),h(H,"class","svelte-10a4hhp"),h(G,"for","fname"),h(F,"type","text"),h(F,"name","pbl"),h(F,"maxlength","75"),h(F,"class","svelte-10a4hhp"),h(J,"class","flex-item svelte-10a4hhp"),h(Q,"class","flex-item svelte-10a4hhp"),h(U,"class","flex-container svelte-10a4hhp"),h(it,"for","fname"),h(at,"type","text"),h(at,"name","geox"),h(at,"class","svelte-10a4hhp"),h(st,"class","flex-item svelte-10a4hhp"),h(ct,"for","lname"),h(ft,"type","text"),h(ft,"name","geoy"),h(ft,"class","svelte-10a4hhp"),h(rt,"class","flex-item svelte-10a4hhp"),h(ot,"class","flex-container svelte-10a4hhp"),h(gt,"class","button button1 svelte-10a4hhp"),h(ht,"class","button_ali svelte-10a4hhp"),h(wt,"style","background-color : #3cbc8d; width: 100%;"),h(wt,"rows","25"),h(wt,"cols","50"),h(n,"class","bg")},m(t,o){a(t,n,o),l(n,s),l(n,i),l(n,d),l(d,v),l(d,m),l(d,b),l(n,x),l(n,$),l(n,y),l(n,w),l(w,C),l(w,k),l(w,_),g(_,e[0].deviceId),l(n,A),l(n,E),l(E,I),l(E,S),l(E,H),g(H,e[0].websocketHost),l(n,P),l(n,L),l(L,G),l(L,T),l(L,F),g(F,e[0].input01.name),l(n,W),l(n,Y),l(n,q),l(n,z),l(z,B),l(z,R),l(z,U),l(U,J);for(let t=0;t<At.length;t+=1)At[t].m(J,null);l(U,K),l(U,Q);for(let t=0;t<It.length;t+=1)It[t].m(Q,null);l(n,V),l(n,X),l(n,Z),l(n,tt),l(tt,et),l(tt,nt),l(tt,ot),l(ot,st),l(st,it),l(st,lt),l(st,at),g(at,e[0].latitude),l(ot,ut),l(ot,rt),l(rt,ct),l(rt,dt),l(rt,ft),g(ft,e[0].longitude),l(tt,pt),l(tt,ht),l(ht,gt),l(n,vt),l(n,mt),l(n,bt),l(n,xt),l(xt,$t),l(xt,yt),l(xt,wt),g(wt,e[0].CACert),Ct||(kt=[p(v,"click",e[2]),p(b,"click",D),p(_,"input",e[3]),p(H,"input",e[4]),p(F,"input",e[5]),p(at,"input",e[8]),p(ft,"input",e[9]),p(gt,"click",e[1]),p(wt,"input",e[10])],Ct=!0)},p(t,[e]){if(1&e&&_.value!==t[0].deviceId&&g(_,t[0].deviceId),1&e&&H.value!==t[0].websocketHost&&g(H,t[0].websocketHost),1&e&&F.value!==t[0].input01.name&&g(F,t[0].input01.name),1&e){let n;for(_t=t[0].ssid,n=0;n<_t.length;n+=1){const o=O(t,_t,n);At[n]?At[n].p(o,e):(At[n]=j(o),At[n].c(),At[n].m(J,null))}for(;n<At.length;n+=1)At[n].d(1);At.length=_t.length}if(1&e){let n;for(Et=t[0].ssid,n=0;n<Et.length;n+=1){const o=N(t,Et,n);It[n]?It[n].p(o,e):(It[n]=M(o),It[n].c(),It[n].m(Q,null))}for(;n<It.length;n+=1)It[n].d(1);It.length=Et.length}1&e&&at.value!==t[0].latitude&&g(at,t[0].latitude),1&e&&ft.value!==t[0].longitude&&g(ft,t[0].longitude),1&e&&g(wt,t[0].CACert)},i:t,o:t,d(t){t&&u(n),r(At,t),r(It,t),Ct=!1,o(kt)}}}function D(){alert("Se guardará la configuracion")}function F(t,e,n){var o={websocketHost:"",CACert:"",latitude:0,longitude:0,deviceId:"",input01:{name:""},ssid:[{ssid:"",pwd:""},{ssid:"",pwd:""},{ssid:"",pwd:""}]};return b((async()=>{})),[o,function(){navigator.geolocation?navigator.geolocation.getCurrentPosition((t=>{n(0,o={latitude:t.coords.latitude,longitude:t.coords.longitude})})):x.innerHTML="Geolocation is not supported by this browser."},async function(){let t=await fetch("/getsettings"),e=await t.json();console.log("Retorna settings",o),e&&(n(0,o={CACert:e.CACert||"",websocketHost:e.websocketHost||"",latitude:e.latitude||0,longitude:e.longitude||0,deviceId:e.deviceId||"",input01:{name:e.input01.name||""}}),n(0,o.ssid=[],o),e.ssid&&Array.isArray(e.ssid)&&e.ssid.forEach((t=>{o.ssid.push({ssid:t.ssid,pwd:t.pwd})})))},function(){o.deviceId=this.value,n(0,o)},function(){o.websocketHost=this.value,n(0,o)},function(){o.input01.name=this.value,n(0,o)},function(){o.ssid[1].ssid=this.value,n(0,o)},function(){o.ssid[0].pwd=this.value,n(0,o)},function(){o.latitude=this.value,n(0,o)},function(){o.longitude=this.value,n(0,o)},function(){o.CACert=this.value,n(0,o)}]}return new class extends class{$destroy(){!function(t,e){const n=t.$$;null!==n.fragment&&(o(n.on_destroy),n.fragment&&n.fragment.d(e),n.on_destroy=n.fragment=null,n.ctx=[])}(this,1),this.$destroy=t}$on(t,e){const n=this.$$.callbacks[t]||(this.$$.callbacks[t]=[]);return n.push(e),()=>{const t=n.indexOf(e);-1!==t&&n.splice(t,1)}}$set(t){var e;this.$$set&&(e=t,0!==Object.keys(e).length)&&(this.$$.skip_bound=!0,this.$$set(t),this.$$.skip_bound=!1)}}{constructor(t){super(),G(this,t,F,T,i,{})}}({target:document.body,props:{name:"Edwin"}})}();)";
        }

        String responseCSS()
        {
            return R"(.flex-container.svelte-10a4hhp{display:-webkit-flex;display:flex;-webkit-align-items:stretch;align-items:stretch;width:auto;height:auto}.flex-item.svelte-10a4hhp{width:50%;margin:10px}input.svelte-10a4hhp{width:100%;padding:12px 20px;margin:8px 0;box-sizing:border-box;border:none;background-color:#3cbc8d;color:white}.button.svelte-10a4hhp{border:none;color:white;padding:16px 32px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;transition-duration:0.4s;cursor:pointer}.button1.svelte-10a4hhp{background-color:white;color:black;border:2px solid #3cbc8d}.button1.svelte-10a4hhp:hover{background-color:#3cbc8d;color:white}.button_ali.svelte-10a4hhp{text-align:end})";
        }

        String responsegetSettings()
        {
            String outputJson = "";
            serializeJson(ocsClass.toJson(), outputJson);
            Serial.println(outputJson);
            return outputJson;
        }
    }

}