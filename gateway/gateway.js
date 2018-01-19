require('dotenv').config()
const spawnSync = require( 'child_process' ).exec;
const EVT = require('evrythng');

var app = new EVT.App(process.env.EVRYTHNG_API_KEY);

setInterval(() => { 
    var ls = spawnSync( '/home/pi/ccn/build/bin/ccn-lite-peek /p/temp | /home/pi/ccn/build/bin/ccn-lite-pktdump -f 2', [], (error, stdout, stderr) => {
        if (!error) {
            console.log('[Update]: Set temperature to', stdout);
            app.product('UnXb9FxxBXPwtpRaagdXfdCb').property().update({
                temperature: stdout
            });
        }
    });
}, 1000);