
$(function() {


  nQuireJsSupport.register('nquireStructureWidget', {
    activityTypes: {
      activity: {
        information: {
          title: 'Information',
          multiplicity: '*',
          requires: []
        },
        notes: {
          title: 'My notes',
          multiplicity: '*',
          requires: []
        },
        hypothesis: {
          title: 'My hypothesis',
          multiplicity: '1',
          requires: []
        },
        hypothesisconclusion: {
          title: 'My hypothesis conclusion',
          multiplicity: '1',
          requires: ['hypothesis']
        },
        keyquestions: {
          title: 'My key questions',
          multiplicity: '1',
          requires: []
        },
        keyanswers: {
          title: 'My key answers',
          multiplicity: '1',
          requires: ['keyquestions']
        },
        methodology: {
          title: 'Decide my methodology',
          multiplicity: '*',
          requires: []
        },
        collectdata: {
          title: 'Collect my data',
          multiplicity: '*',
          requires: []
        },
        exploretool: {
          title: 'Explore a scientific instrument',
          multiplicity: '*',
          requires: []
        },
        analysedata: {
          title: 'Analyse my data',
          multiplicity: '*',
          requires: []
        },
        spreadsheet: {
          title: 'Data spreadsheet analysis',
          multiplicity: '*',
          requires: []
        }
      },
      phase: {
        introduction: {
          title: 'Introduction',
          activities: ['notes']
        },
        questions: {
          title: 'My hypothesis',
          activities: ['hypothesis', 'keyquestions']
        },
        methodology: {
          title: 'Methodology',
          activities: ['methodology']
        },
        collectdata: {
          title: 'Data gathering',
          activities: ['collectdata']
        },
        analysedata: {
          title: 'Data analysis',
          activities: ['analysedata']
        },
        conclusions: {
          title: 'My conclusions',
          activities: ['hypothesisconclusion', 'keyanswers']
        },
        empty: {
          title: 'Empty phase',
          activities: []
        }
      }
    },
    init: function() {
      var self = this;
      $("div[nquire-widget='inquiry-structure']").each(function() {
        $(this).nquireStructureWidget({
          availableActivities: self.activityTypes
        });
      });
    }
  });
});