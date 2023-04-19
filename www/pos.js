proj4.defs(
    "EPSG:3765",
    "+proj=tmerc +lat_0=0 +lon_0=16.5 +k=0.9999 +x_0=500000 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs"
);

function badgeOnOff(id, isOn) {
    if (isOn) {
        document.getElementById(id).className = "badge badge-primary";
    } else {
        document.getElementById(id).className = "badge badge-secondary";
    }
}

var latitude = $("#latitude");

function set_fixed_val(obj, val, d) {
    if (typeof val !== 'undefined') {
        if (d > -1) {
            $(obj).text(val.toFixed(d));
        } else {
            $(obj).text(val);
        }
    } else {
        $(obj).text("n/a");
    }
}

var posUpdate = function() {
    $.ajax({
            url: "ubx/pos",
            dataType: "json",
            timeout: 2000,
        })
        .done(function(data) {
            document.getElementById("fixType").textContent = data.fixType;
            badgeOnOff("gnssFixOK", data.gnssFixOK);
            if (data.corrSoln == 0) {
                $("#rtk").text("No RTK");
                document.getElementById("rtk").className =
                    "badge badge-secondary";
            } else if (data.corrSoln == 1) {
                $("#rtk").text("RTK Float");
                document.getElementById("rtk").className = "badge badge-warning";
            } else if (data.corrSoln == 2) {
                $("#rtk").text("RTK Fixed");
                document.getElementById("rtk").className = "badge badge-primary";
            }
            set_fixed_val("#latitude", data.lat, -1);
            set_fixed_val("#longitude", data.lon, -1);
            set_fixed_val("#height", data.height, 3);
            set_fixed_val("#hMSL", data.hMSL, 3);
            set_fixed_val("#hAcc", data.hAcc, 3);
            set_fixed_val("#vAcc", data.vAcc, 3);
            set_fixed_val("#sss", data.usss, 3);
            set_fixed_val("#ttff", data.ttff, 3);
            var katastar_url = document.getElementById("katastar_url");
            if (typeof data.lon !== 'undefined' && typeof data.lat !== 'undefined') {
                htrs = proj4("EPSG:4326", "EPSG:3765", [data.lon, data.lat]);
                $("#htrs").text(htrs[0].toFixed(3) + ", " + htrs[1].toFixed(2));
                katastar_url.href = "https://oss.uredjenazemlja.hr/map?center=" + htrs[0].toFixed(3) + "," + htrs[1].toFixed(3) + "&zoom=22";
            } else {
                $("#htrs").text("n/a");
                katastar_url.href = "/#";
            }
        })
        .always(function() {
            setTimeout(posUpdate, 2500);
        });
};
posUpdate();
