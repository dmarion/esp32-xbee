import './style.css';
import ImageLayer from 'ol/layer/Image.js';
import ImageWMS from 'ol/source/ImageWMS.js';
import GeoJSON from 'ol/format/GeoJSON.js';
import { Map, View, Feature } from 'ol';
import { LineString, Point } from 'ol/geom.js';
import MultiPoint from 'ol/geom/MultiPoint.js';
import Projection from 'ol/proj/Projection.js';
import { OSM, Vector as VectorSource } from 'ol/source.js';
import { Tile as TileLayer, Vector as VectorLayer } from 'ol/layer.js';
import { Circle as CircleStyle, Fill, Stroke, Style } from 'ol/style.js';
import { ScaleLine, defaults as defaultControls } from 'ol/control.js';
import { fromLonLat, transformExtent } from 'ol/proj.js';
import { get as getProjection, getTransform } from 'ol/proj.js';
import { getVectorContext } from 'ol/render.js';
import { register } from 'ol/proj/proj4.js';
import proj4 from 'proj4';

function el(id) {
    return document.getElementById(id);
}

proj4.defs(
    "EPSG:3765",
    "+proj=tmerc +lat_0=0 +lon_0=16.5 +k=0.9999 +x_0=500000 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs"
);
register(proj4);

const center = [332525, 5026887];

const projection = getProjection('EPSG:3765')

const osm = new TileLayer({
    source: new OSM(),
});

const of2020 = new ImageLayer({
    source: new ImageWMS({
        url: 'https://geoportal.dgu.hr/services/inspire/orthophoto_2019_2020/ows',
        params: {
            'LAYERS': 'OI.OrthoimageCoverage',
            'TRANSPARENT': true
        },
        ratio: 1,
        serverType: 'geoserver',
    })
});

const style = new Style({
    fill: new Fill({
        color: '#eeeeee',
    }),
});

const styles = [
    new Style({
        stroke: new Stroke({
            color: 'blue',
            width: 3,
        }),
        fill: new Fill({
            color: 'rgba(0, 0, 255, 0.1)',
        }),
    }),
    new Style({
        image: new CircleStyle({
            radius: 5,
            fill: new Fill({
                color: 'red',
            }),
        }),
        geometry: function(feature) {
            // return the coordinates of the first ring of the polygon
            const coordinates = feature.getGeometry().getCoordinates()[0];
            return new MultiPoint(coordinates);
        },
    }),
];

const parceleSource = new VectorSource({
    url: 'parcele.js',
    format: new GeoJSON(),
});


const parcele = new VectorLayer({
    //background: '#1a2b39',
    source: parceleSource,
    style: styles, //function (feature) { const color = feature.get('COLOR') || '#ff0000'; style.getFill().setColor(color); return style; },
});

const map = new Map({
    controls: defaultControls().extend([new ScaleLine()]),
    target: 'map',
    layers: [osm, parcele],
    view: new View({
        projection: projection,
        //center: center,
        zoom: 9,
        //extent: transformExtent(bbox, "EPSG:3765", "HTRS96/TM"),
    })
});

const posFeature = new Feature();
posFeature.setStyle(
    new Style({
        image: new CircleStyle({
            radius: 6,
            fill: new Fill({
                color: '#3399CC',
            }),
            stroke: new Stroke({
                color: '#fff',
                width: 2,
            }),
        }),
    })
);

new VectorLayer({
    map: map,
    source: new VectorSource({
        features: [posFeature],
    }),
});


let point = null;
let line = null;

function pos_update(data) {
    console.log(data);
    if (typeof data.lon == 'undefined' || typeof data.lat == 'undefined') {
        return;
    }
    let htrs = proj4("EPSG:4326", "EPSG:3765", [data.lon, data.lat]);
    var [n, e] = htrs;
    var v = map.getView();
    console.log("N = " + n + ", E = " + e);

    v.setCenter(htrs);
    if (v.getZoom() < 19) {
        v.setZoom(19);
    }
    posFeature.setGeometry(new Point(htrs));

    const cf = parceleSource.getClosestFeatureToCoordinate(htrs);
    if (cf === null) {
        point = null;
        line = null;
    } else {
        const mp = new MultiPoint(cf.getGeometry().getCoordinates()[0]);
        const cp = mp.getClosestPoint(htrs);
        if (point === null) {
            point = new Point(cp);
        } else {
            point.setCoordinates(cp);
        }
        const coordinates = [htrs, [cp[0], cp[1]]];
        console.log(coordinates);
        if (line === null) {
            line = new LineString(coordinates);
        } else {
            line.setCoordinates(coordinates);
        }
    }
    map.render();

    let d;
    if (line !== null) {
        d = "d = " + line.getLength().toFixed(2);
    } else {
        d = "d = n/a";
    }
    el('info').innerHTML = [
        data.lat, data.lon, htrs[0].toFixed(3), htrs[1].toFixed(3), d
    ].join('<br />');
}

const stroke = new Stroke({
    color: 'rgba(255,0,0,0.9)',
    width: 1,
});
const style2 = new Style({
    stroke: stroke,
    image: new CircleStyle({
        radius: 5,
        fill: null,
        stroke: stroke,
    }),
});

parcele.on('postrender', function(evt) {
    const vectorContext = getVectorContext(evt);
    vectorContext.setStyle(style2);
    if (point !== null) {
        vectorContext.drawGeometry(point);
    }
    if (line !== null) {
        vectorContext.drawGeometry(line);
    }
});

async function update() {
    fetch('/ubx/pos')
        .then((response) => response.json())
        .then((json) => pos_update(json));
}

window.setInterval(update, 3000);
