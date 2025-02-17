
        var Socket; 
        var c = document.getElementById('myCanvas');
        var ctx = c.getContext('2d');
        var previousData = {"initialized":false, "dataCounter":0};
        var settings = {};
        var warning = 115;
        var danger = 125;
        var shiftLight = 6500;
        var startType = true;
        var startMode = "Sport";
        var _blue_cool = "rgb(0, 191, 255)";
        var _blue_daylight = "rgb(50, 115, 236)";
        var _green = "rgb(26, 255, 26)";
        var _RPM_orange = "rgb(247, 166, 2)";
        // var timeoutId = setTimeout(function() {
        // location.reload();
        // }, 4000);

        window.onoffline = function() {
          console.log("Page disconnected");
          location.reload();
          // Add your code here to handle the disconnection
        };

        document.getElementById("settings-button").addEventListener("click",function() {
        var settingsContainer = document.getElementById("settings-container");
        previousData.initialized = false;
        if (settingsContainer.style.display === "none") {
          settingsContainer.style.display = "block";
          var allElements = document.querySelectorAll('body > :not(#settings-container)');
          allElements.forEach(function(element) {
            element.style.display = "none";
            
          });
        } else {
          settingsContainer.style.display = "none";
          var allElements = document.querySelectorAll('body > :not(#settings-container)');
          allElements.forEach(function(element) {
            element.style.display = "block";
          });
        }
        });

        document.getElementById("save-button").addEventListener("click", function() {
          var settingsContainer = document.getElementById("settings-container");
          if (settingsContainer.style.display === "block") {
            settingsContainer.style.display = "none";
          var allElements = document.querySelectorAll('body > :not(#settings-container)');
            allElements.forEach(function(element) {
              element.style.display = "block";
          });
          } 
          else {
          settingsContainer.style.display = "block";
          var allElements = document.querySelectorAll('body > :not(#settings-container)');
          allElements.forEach(function(element) {
            element.style.display = "none";
          });
        }
          const jsonData = JSON.stringify(settings);
          console.log('Sending data to server:', jsonData);
          Socket.send(jsonData);
        });

        document.getElementById("start-up-type").addEventListener("change", function() {
          var startUpType = document.getElementById("start-up-type").value;
          if (startUpType === "start-from-set-mode") {
            document.getElementById("start-up-mode").style.display = "inline";
            settings.sType = 1;
            
          } else {
            document.getElementById("start-up-mode").style.display = "none";
            settings.sType = 0;
          }
        });

        document.getElementById("start-up-mode-select").addEventListener("change", function() {
          settings.sMode = document.getElementById("start-up-mode-select").value;
        });


        document.getElementById("shiftlight-rpm").addEventListener("input", function() {
          document.getElementById("shiftlight-rpm-value").innerHTML = document.getElementById("shiftlight-rpm").value;
          settings.shftRPM = document.getElementById("shiftlight-rpm").value;
        });

        document.getElementById("oil-warning-temp").addEventListener("input", function() {
          document.getElementById("oil-warning-temp-value").innerHTML = document.getElementById("oil-warning-temp").value;
          settings.OWT = document.getElementById("oil-warning-temp").value;
        });

        document.getElementById("oil-danger-temp").addEventListener("input", function() {
          document.getElementById("oil-danger-temp-value").innerHTML = document.getElementById("oil-danger-temp").value;
          settings.ODT = document.getElementById("oil-danger-temp").value;
        });

        document.getElementById("shift_reminder").addEventListener("input", function() {
          document.getElementById("shift_reminder_value").innerHTML = document.getElementById("shift_reminder").value;
          settings.rem_sec = document.getElementById("shift_reminder").value;
        });

        document.getElementById("theme-switcher")?.addEventListener("click", () => {
          document.documentElement.classList.toggle("dark-theme");});

        function init() { 
          Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
          Socket.onmessage = function (event) {
            if (Socket.readyState === WebSocket.OPEN) {
              processReceivedCommand(event);}
          }
  		  };

        function processReceivedCommand(event) {
          const obj = JSON.parse(event.data);
          if (!previousData.initialized) {
            previousData.initialized = true;
            // console.log("settings initialized");

            settings.sType = obj.startType;
            if (obj.startType === 1) {
              document.getElementById("start-up-type").value = "Start from set mode";
            }
            else {
              document.getElementById("start-up-type").value = "Start from last mode";
            }
            settings.sMode = obj.startMode;
            document.getElementById("start-up-mode").value = obj.startMode;

            settings.shftRPM = obj.shiftRPM;
            document.getElementById("shiftlight-rpm-value").innerHTML = obj.shiftRPM;
            document.getElementById("shiftlight-rpm").value = obj.shiftRPM;

            settings.OWT = obj.warning_temp;
            document.getElementById("oil-warning-temp-value").innerHTML = obj.warning_temp;
            document.getElementById("oil-warning-temp").value = obj.warning_temp;
          
            settings.ODT = obj.danger_temp; 
            document.getElementById("oil-danger-temp-value").innerHTML = obj.danger_temp;
            document.getElementById("oil-danger-temp").value = obj.danger_temp;
            settings.rem_sec = obj.reminder_sec;

            document.getElementById("shift_reminder_value").innerHTML = settings.rem_sec;
            document.getElementById("shift_reminder").value = obj.reminder_sec;
            // console.log(settings);
          }
          // previousData.dataCounter++;
          // if (previousData.dataCounter > 8) {
          //   previousData.dataCounter = 0;
          //   clearTimeout(timeoutId);
          //   console.log("heartbeat");
          //   timeoutId = setTimeout(function() {
          //     console.log("timeout");
          //   location.reload();

          // }, 1500);
          // }


          var darkStatus = document.documentElement.classList.contains("dark-theme");

          if (previousData.lightsOn !== obj.lightsOn && obj.lightsOn !== darkStatus) {
            document.documentElement.classList.toggle("dark-theme");
            darkStatus = !darkStatus;
          }
          // Update only if values change

          if (previousData.Gear !== obj.Gear 
              || previousData.shiftLight !== obj.shiftLight 
              || previousData.remind !==obj.remind
              ||previousData.DarkStatus !== darkStatus) {
                document.getElementById("Gear").innerHTML = obj.Gear;
                document.getElementById("Gear").style.color = obj.shiftLight? "red":
                                                              obj.remind? _green:
                                                              darkStatus? "black":"white";
                                                            }
    
          if (previousData.oil_temp !== obj.oil_temp
              || previousData.warning !== obj.warning
              || previousData.danger !== obj.danger
              || previousData.DarkStatus !== darkStatus) {
                document.getElementById("oil_temp").innerHTML = obj.oil_temp;
                document.getElementById("oil_temp").style.color =
                obj.danger?     "red": 
                obj.warning?    "yellow":
                obj.coldEngine? _blue_cool: 
                darkStatus?     "black":"white";
              }
    
          if (previousData.water_temp !== obj.water_temp 
              || previousData.coldEngine !== obj.coldEngine
              || previousData.DarkStatus !== darkStatus) {
                document.getElementById("water_temp").innerHTML = obj.water_temp;
                document.getElementById("water_temp").style.color =
                obj.water_temp >= 115? "red":
                obj.water_temp >= 110? "yellow": 
                obj.water_temp < 75? _blue_cool:         
                darkStatus? "black":"white";
              }
    
          if (previousData.gearbox_temp !== obj.gearbox_temp 
              || previousData.coldEngine !== obj.coldEngine
              || previousData.DarkStatus !== darkStatus) {
                document.getElementById("gearbox_temp").innerHTML = obj.gearbox_temp;
                document.getElementById("gearbox_temp").style.color =
                obj.gearbox_temp < 45? _blue_cool: 
                obj.gearbox_temp > 100? "yellow":         
                darkStatus? "black":"white";
              }
    
          if (previousData.speed !== obj.speed) {
            document.getElementById("speed").innerHTML = obj.speed;
          }
          
          if (previousData.ethContent !== obj.ethContent) {
            document.getElementById("ethContent").innerHTML = obj.ethContent;
          }
          
          if (previousData.driveMode !== obj.driveMode) {
            document.getElementById("driveMode").innerHTML = obj.driveMode;
            let _color = "white";
            switch (obj.driveMode) {
              
              case ("ECO"):
                _color = "rgb(26, 26, 255)";
                break;

              case ("Comfort"):
                _color = "rgb(0, 204, 255)";
                break;

              case ("Sport"):
                _color = "rgb(255, 153, 102)";
                break;

              case ("Sport +"):
                _color = "rgb(255, 0, 0)";
                break;

              case ("Traction"):
                _color = "rgb(230, 230, 0)";
                break;

              case ("DSC Off"):
                _color = "rgb(255, 255, 255)";
                break;
            }
            document.getElementById("driveMode").style.color = _color;
          }
        
          if (Math.abs(previousData.RPM - obj.RPM) > 10) {const grd = ctx.createLinearGradient(obj.RPM / 10 - 35, 0, obj.RPM / 10, 0);
            if (obj.shiftLight){
              grd.addColorStop(0, "red");
            }
            else if (darkStatus) {
              grd.addColorStop(0, _RPM_orange);
            }
            else {
              grd.addColorStop(0, _blue_daylight);
            }
            grd.addColorStop(1, "rgb(0,0,0)");
            ctx.clearRect(0, 0, c.width, c.height); // Clear canvas before redrawing
            ctx.fillStyle = grd;
            ctx.fillRect(0, 0, 680, 30);
        }

        // previousData = obj;
        for (const key in obj) {
          previousData[key] = obj[key];
        }
      }
        window.onload = function(event) {
          init();
        }
