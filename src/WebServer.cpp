#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

namespace ocs
{

    class WebAdmin : public AsyncWebServer
    {

    public:
        WebAdmin(int port) : AsyncWebServer(port)
        {
            Serial.printf("WebAdmin Port: %i\n", port);
        }

#ifdef ESP32
        void setup()
        {

            // Initialize LittleFS
            if (!LittleFS.begin())
            {
                Serial.println("An Error has occurred while mounting LittleFS");
                return;
            }
            /*
                        this->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
                                 {Serial.println(ESP.getFreeHeap(), DEC); request->send(200, F("text/html"), this->responseRoot()); });
            */

            this->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/index.html"), F("text/html")); });

            this->on("/build/bundle.css", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/bundle.css"), F("text/css")); });

            this->on("/build/bundle.js", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/bundle.js"), F("application/javascript")); });
            /*
                        this->on("/build/bundle.css", HTTP_GET, [&](AsyncWebServerRequest *request)
                                 {Serial.println(ESP.getFreeHeap(), DEC); request->send(200, F("text/css"), this->responseCSS()); });
            */
            // Serial.println(this->responsebundleJS());
        }

#elif defined(ESP8266)
        void setup()
        {

            // Initialize LittleFS
            if (!LittleFS.begin())
            {
                Serial.println("An Error has occurred while mounting LittleFS");
                // return;
            }

            this->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/index.html"), F("text/html")); });

            this->on("/build/bundle.css", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/bundle.css"), F("text/css")); });

            this->on("/build/bundle.js", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/bundle.js"), F("application/javascript")); });
            this->onNotFound([&](AsyncWebServerRequest *request)
                             { request->send(404, F("text/plain"), "Not found"); });
        }

#endif

    private:
    /*
        void notFound(AsyncWebServerRequest *request)
        {
            request->send(404, F("text/plain"), "Not found");
        }
        */

        /*
            const char *responseRoot()
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
    */

        /*
                const char *responsebundleJS()
                {
                    return R"(var app=function(){"use strict";function t(){}function e(t){return t()}function n(){return Object.create(null)}function o(t){t.forEach(e)}function s(t){return"function"==typeof t}function i(t,e){return t!=t?e==e:t!==e||t&&"object"==typeof t||"function"==typeof t}function l(t,e){t.appendChild(e)}function a(t,e,n){t.insertBefore(e,n||null)}function u(t){t.parentNode.removeChild(t)}function c(t,e){for(let n=0;n<t.length;n+=1)t[n]&&t[n].d(e)}function r(t){return document.createElement(t)}function d(t){return document.createTextNode(t)}function f(){return d(" ")}function p(t,e,n,o){return t.addEventListener(e,n,o),()=>t.removeEventListener(e,n,o)}function h(t,e,n){null==n?t.removeAttribute(e):t.getAttribute(e)!==n&&t.setAttribute(e,n)}function g(t,e){t.value=null==e?"":e}let m;function v(t){m=t}function b(t){(function(){if(!m)throw new Error("Function called outside component initialization");return m})().$$.on_mount.push(t)}const $=[],w=[],x=[],y=[],C=Promise.resolve();let _=!1;function k(t){x.push(t)}const A=new Set;let I=0;function S(){const t=m;do{for(;I<$.length;){const t=$[I];I++,v(t),E(t.$$)}for(v(null),$.length=0,I=0;w.length;)w.pop()();for(let t=0;t<x.length;t+=1){const e=x[t];A.has(e)||(A.add(e),e())}x.length=0}while($.length);for(;y.length;)y.pop()();_=!1,A.clear(),v(t)}function E(t){if(null!==t.fragment){t.update(),o(t.before_update);const e=t.dirty;t.dirty=[-1],t.fragment&&t.fragment.p(t.ctx,e),t.after_update.forEach(k)}}const H=new Set;function P(t,e){-1===t.$$.dirty[0]&&($.push(t),_||(_=!0,C.then(S)),t.$$.dirty.fill(0)),t.$$.dirty[e/31|0]|=1<<e%31}function O(i,l,a,c,r,d,f,p=[-1]){const h=m;v(i);const g=i.$$={fragment:null,ctx:null,props:d,update:t,not_equal:r,bound:n(),on_mount:[],on_destroy:[],on_disconnect:[],before_update:[],after_update:[],context:new Map(l.context||(h?h.$$.context:[])),callbacks:n(),dirty:p,skip_bound:!1,root:l.target||h.$$.root};f&&f(g.root);let b=!1;if(g.ctx=a?a(i,l.props||{},((t,e,...n)=>{const o=n.length?n[0]:e;return g.ctx&&r(g.ctx[t],g.ctx[t]=o)&&(!g.skip_bound&&g.bound[t]&&g.bound[t](o),b&&P(i,t)),e})):[],g.update(),b=!0,o(g.before_update),g.fragment=!!c&&c(g.ctx),l.target){if(l.hydrate){const t=function(t){return Array.from(t.childNodes)}(l.target);g.fragment&&g.fragment.l(t),t.forEach(u)}else g.fragment&&g.fragment.c();l.intro&&(($=i.$$.fragment)&&$.i&&(H.delete($),$.i(w))),function(t,n,i,l){const{fragment:a,on_mount:u,on_destroy:c,after_update:r}=t.$$;a&&a.m(n,i),l||k((()=>{const n=u.map(e).filter(s);c?c.push(...n):o(n),t.$$.on_mount=[]})),r.forEach(k)}(i,l.target,l.anchor,l.customElement),S()}var $,w;v(h)}function j(t,e,n){const o=t.slice();return o[13]=e[n].wifi,o[14]=e,o[15]=n,o}function G(t,e,n){const o=t.slice();return o[13]=e[n].wifi,o[16]=e,o[15]=n,o}function N(t){let e,n,o,s,i,c,m,v=t[15]+1+"";function b(){t[7].call(i,t[15])}return{c(){e=r("label"),n=d("SSID "),o=d(v),s=f(),i=r("input"),h(e,"for","fname"),h(i,"type","text"),h(i,"maxlength","15"),h(i,"class","svelte-10a4hhp")},m(u,r){a(u,e,r),l(e,n),l(e,o),a(u,s,r),a(u,i,r),g(i,t[0].ssid[t[15]].ssid),c||(m=p(i,"input",b),c=!0)},p(e,n){t=e,1&n&&i.value!==t[0].ssid[t[15]].ssid&&g(i,t[0].ssid[t[15]].ssid)},d(t){t&&u(e),t&&u(s),t&&u(i),c=!1,m()}}}function D(t){let e,n,o,s,i,c,m,v=t[15]+1+"";function b(){t[8].call(i,t[15])}return{c(){e=r("label"),n=d("Password "),o=d(v),s=f(),i=r("input"),h(e,"for","lname"),h(i,"type","password"),h(i,"class","svelte-10a4hhp")},m(u,r){a(u,e,r),l(e,n),l(e,o),a(u,s,r),a(u,i,r),g(i,t[0].ssid[t[15]].pwd),c||(m=p(i,"input",b),c=!0)},p(e,n){t=e,1&n&&i.value!==t[0].ssid[t[15]].pwd&&g(i,t[0].ssid[t[15]].pwd)},d(t){t&&u(e),t&&u(s),t&&u(i),c=!1,m()}}}function L(e){let n,s,i,m,v,b,$,w,x,y,C,_,k,A,I,S,E,H,P,O,L,T,M,F,W,Y,q,z,B,J,R,U,X,K,Q,V,Z,tt,et,nt,ot,st,it,lt,at,ut,ct,rt,dt,ft,pt,ht,gt,mt,vt,bt,$t,wt,xt,yt,Ct,_t,kt,At,It,St,Et=e[0].ssid,Ht=[];for(let t=0;t<Et.length;t+=1)Ht[t]=N(G(e,Et,t));let Pt=e[0].ssid,Ot=[];for(let t=0;t<Pt.length;t+=1)Ot[t]=D(j(e,Pt,t));return{c(){n=r("div"),s=r("h1"),s.textContent="OPEN COMMUNITY SAFETY",i=f(),m=r("div"),v=r("button"),v.textContent="Get settings",b=f(),$=r("button"),$.textContent="Save settings",w=f(),x=r("h2"),x.textContent="Settings",y=f(),C=r("div"),_=r("label"),_.textContent="Device ID",k=f(),A=r("input"),I=f(),S=r("div"),E=r("label"),E.textContent="Websocket Host",H=f(),P=r("input"),O=f(),L=r("div"),T=r("label"),T.textContent="Physical button label",M=f(),F=r("input"),W=f(),Y=r("br"),q=f(),z=r("div"),B=r("h3"),B.textContent="WIFI",J=f(),R=r("div"),U=r("div");for(let t=0;t<Ht.length;t+=1)Ht[t].c();X=f(),K=r("div");for(let t=0;t<Ot.length;t+=1)Ot[t].c();var t,o,l,a;Q=f(),V=r("br"),Z=f(),tt=r("div"),et=r("h3"),et.textContent="Geolocation",nt=f(),ot=r("div"),st=r("div"),it=r("label"),it.textContent="Latitude",lt=f(),at=r("input"),ut=f(),ct=r("div"),rt=r("label"),rt.textContent="Longitude",dt=f(),ft=r("input"),pt=f(),ht=r("div"),gt=r("button"),gt.textContent="Get Geolocation",mt=f(),vt=r("a"),bt=d("Show map"),wt=f(),xt=r("br"),yt=f(),Ct=r("div"),_t=r("h3"),_t.textContent="SSL Certificate",kt=f(),At=r("textarea"),t=s,o="color",null===(l="darkcyan")?t.style.removeProperty(o):t.style.setProperty(o,l,a?"important":""),h(v,"class","button button1 svelte-10a4hhp"),h($,"class","button button1 svelte-10a4hhp"),h(m,"class","button_ali svelte-10a4hhp"),h(_,"for","fname"),h(A,"type","text"),h(A,"name","deviceId"),h(A,"maxlength","40"),h(A,"class","svelte-10a4hhp"),h(E,"for","fname"),h(P,"type","text"),h(P,"name","websockethost"),h(P,"class","svelte-10a4hhp"),h(T,"for","fname"),h(F,"type","text"),h(F,"name","pbl"),h(F,"maxlength","50"),h(F,"class","svelte-10a4hhp"),h(U,"class","flex-item svelte-10a4hhp"),h(K,"class","flex-item svelte-10a4hhp"),h(R,"class","flex-container svelte-10a4hhp"),h(it,"for","fname"),h(at,"type","text"),h(at,"name","geox"),h(at,"class","svelte-10a4hhp"),h(st,"class","flex-item svelte-10a4hhp"),h(rt,"for","lname"),h(ft,"type","text"),h(ft,"name","geoy"),h(ft,"class","svelte-10a4hhp"),h(ct,"class","flex-item svelte-10a4hhp"),h(ot,"class","flex-container svelte-10a4hhp"),h(gt,"class","button button1 svelte-10a4hhp"),h(ht,"class","button_ali svelte-10a4hhp"),h(vt,"target","_blank"),h(vt,"href",$t=`https://www.openstreetmap.org/?mlat=${e[0].latitude}&mlon=${e[0].longitude}#map=19/${e[0].latitude}/${e[0].longitude}`),h(At,"style","background-color : #3cbc8d; width: 100%;"),h(At,"rows","25"),h(At,"cols","50"),h(n,"class","bg")},m(t,o){a(t,n,o),l(n,s),l(n,i),l(n,m),l(m,v),l(m,b),l(m,$),l(n,w),l(n,x),l(n,y),l(n,C),l(C,_),l(C,k),l(C,A),g(A,e[0].deviceId),l(n,I),l(n,S),l(S,E),l(S,H),l(S,P),g(P,e[0].websocketHost),l(n,O),l(n,L),l(L,T),l(L,M),l(L,F),g(F,e[0].input01.name),l(n,W),l(n,Y),l(n,q),l(n,z),l(z,B),l(z,J),l(z,R),l(R,U);for(let t=0;t<Ht.length;t+=1)Ht[t].m(U,null);l(R,X),l(R,K);for(let t=0;t<Ot.length;t+=1)Ot[t].m(K,null);l(n,Q),l(n,V),l(n,Z),l(n,tt),l(tt,et),l(tt,nt),l(tt,ot),l(ot,st),l(st,it),l(st,lt),l(st,at),g(at,e[0].latitude),l(ot,ut),l(ot,ct),l(ct,rt),l(ct,dt),l(ct,ft),g(ft,e[0].longitude),l(tt,pt),l(tt,ht),l(ht,gt),l(tt,mt),l(tt,vt),l(vt,bt),l(n,wt),l(n,xt),l(n,yt),l(n,Ct),l(Ct,_t),l(Ct,kt),l(Ct,At),g(At,e[0].CACert),It||(St=[p(v,"click",e[3]),p($,"click",e[2]),p(A,"input",e[4]),p(P,"input",e[5]),p(F,"input",e[6]),p(at,"input",e[9]),p(ft,"input",e[10]),p(gt,"click",e[1]),p(At,"input",e[11])],It=!0)},p(t,[e]){if(1&e&&A.value!==t[0].deviceId&&g(A,t[0].deviceId),1&e&&P.value!==t[0].websocketHost&&g(P,t[0].websocketHost),1&e&&F.value!==t[0].input01.name&&g(F,t[0].input01.name),1&e){let n;for(Et=t[0].ssid,n=0;n<Et.length;n+=1){const o=G(t,Et,n);Ht[n]?Ht[n].p(o,e):(Ht[n]=N(o),Ht[n].c(),Ht[n].m(U,null))}for(;n<Ht.length;n+=1)Ht[n].d(1);Ht.length=Et.length}if(1&e){let n;for(Pt=t[0].ssid,n=0;n<Pt.length;n+=1){const o=j(t,Pt,n);Ot[n]?Ot[n].p(o,e):(Ot[n]=D(o),Ot[n].c(),Ot[n].m(K,null))}for(;n<Ot.length;n+=1)Ot[n].d(1);Ot.length=Pt.length}1&e&&at.value!==t[0].latitude&&g(at,t[0].latitude),1&e&&ft.value!==t[0].longitude&&g(ft,t[0].longitude),1&e&&$t!==($t=`https://www.openstreetmap.org/?mlat=${t[0].latitude}&mlon=${t[0].longitude}#map=19/${t[0].latitude}/${t[0].longitude}`)&&h(vt,"href",$t),1&e&&g(At,t[0].CACert)},i:t,o:t,d(t){t&&u(n),c(Ht,t),c(Ot,t),It=!1,o(St)}}}function T(t,e,n){var o=3,s={websocketHost:"",CACert:"",latitude:0,longitude:0,deviceId:"",input01:{name:""},ssid:[{ssid:"",pwd:""},{ssid:"",pwd:""},{ssid:"",pwd:""}]};return b((async()=>{})),[s,function(){navigator.geolocation?navigator.geolocation.getCurrentPosition((t=>{n(0,s={latitude:t.coords.latitude,longitude:t.coords.longitude})})):alert("Geolocation is not supported by this browser.")},async function(){if(confirm("Desea guardar los cambios?"))try{let t=await fetch("/setsettings",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(s)}),e=await t.json();console.log(e),e&&alert("Guardado")}catch(t){console.log(t)}},async function(){let t=await fetch("/getsettings"),e=await t.json();if(console.log("Retorna settings",s),e){o=e.MAX_SSID||3,n(0,s={CACert:e.CACert||"",websocketHost:e.websocketHost||"",latitude:e.latitude||0,longitude:e.longitude||0,deviceId:e.deviceId||"",input01:{name:e.input01.name||""}}),n(0,s.ssid=[],s),e.ssid&&Array.isArray(e.ssid)&&e.ssid.forEach((t=>{s.ssid.push({ssid:t.ssid,pwd:t.pwd})}));let t=o+1;s.ssid&&(t=o-s.ssid.length),console.log(s,t);let i=0;for(;i<t;)s.ssid.push({ssid:"",pwd:""}),i++;console.log("settings: ",s)}},function(){s.deviceId=this.value,n(0,s)},function(){s.websocketHost=this.value,n(0,s)},function(){s.input01.name=this.value,n(0,s)},function(t){s.ssid[t].ssid=this.value,n(0,s)},function(t){s.ssid[t].pwd=this.value,n(0,s)},function(){s.latitude=this.value,n(0,s)},function(){s.longitude=this.value,n(0,s)},function(){s.CACert=this.value,n(0,s)}]}return new class extends class{$destroy(){!function(t,e){const n=t.$$;null!==n.fragment&&(o(n.on_destroy),n.fragment&&n.fragment.d(e),n.on_destroy=n.fragment=null,n.ctx=[])}(this,1),this.$destroy=t}$on(t,e){const n=this.$$.callbacks[t]||(this.$$.callbacks[t]=[]);return n.push(e),()=>{const t=n.indexOf(e);-1!==t&&n.splice(t,1)}}$set(t){var e;this.$$set&&(e=t,0!==Object.keys(e).length)&&(this.$$.skip_bound=!0,this.$$set(t),this.$$.skip_bound=!1)}}{constructor(t){super(),O(this,t,T,L,i,{})}}({target:document.body,props:{name:"Edwin"}})}();)";
                }
        */
        /*
         const char *responseCSS()
         {
             return R"(.flex-container.svelte-10a4hhp{display:-webkit-flex;display:flex;-webkit-align-items:stretch;align-items:stretch;width:auto;height:auto}.flex-item.svelte-10a4hhp{width:50%;margin:10px}input.svelte-10a4hhp{width:100%;padding:12px 20px;margin:8px 0;box-sizing:border-box;border:none;background-color:#3cbc8d;color:white}.button.svelte-10a4hhp{border:none;color:white;padding:16px 32px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;transition-duration:0.4s;cursor:pointer}.button1.svelte-10a4hhp{background-color:white;color:black;border:2px solid #3cbc8d}.button1.svelte-10a4hhp:hover{background-color:#3cbc8d;color:white}.button_ali.svelte-10a4hhp{text-align:end})";
         }
         */
    };
}