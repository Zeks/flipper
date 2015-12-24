#ifndef HEADERVIEWSAVER_H
#define HEADERVIEWSAVER_H
#include <QString>
#include <QStringList>
#include <QHash>
#include <QObject>
#include <QTextCodec>
#include <QHeaderView>
#include <QTableView>
#include <QSettings>


struct QHeaderViewState
  {

    struct HeaderState
    {
      bool hidden = false;
      int size = 0;
      int logical_index = 0;
      int visual_index = 0;
      QString name;
      QTableView const  *view;

      HeaderState():view(0){}

      void SaveState(int i, QString section)
      {
          QSettings file("Settings/arm_guistate.ini", QSettings::IniFormat);
          file.setIniCodec(QTextCodec::codecForName("UTF-8"));
          QString headerName = section + "/header" + QString::number(i);
          file.setValue(headerName + "/hidden", hidden);
          file.setValue(headerName + "/size", size);
          file.setValue(headerName + "/logical_index", logical_index);
          file.setValue(headerName + "/visual_index", visual_index);
          file.setValue(headerName + "/name", name);
      }
    };


    explicit QHeaderViewState(QTableView const & view):
      m_headers(view.horizontalHeader()->count())
    {
      QHeaderView const & headers(*view.horizontalHeader());
      // Stored in *visual index* order
      for(int vi = 0; vi < headers.count();++vi)
      {
        int           li     = headers.logicalIndex(vi);
        HeaderState & header = m_headers[vi];

        header.hidden               = headers.isSectionHidden(li);
        header.size                 = headers.sectionSize(li);
        header.logical_index        = li;
        header.visual_index         = vi;
        header.name                 = view.model()->headerData(li,Qt::Horizontal).toString();
        header.view                 = &view;
      }
    }

    void SaveState(QString section)
    {
        QSettings file("Settings/arm_guistate.ini", QSettings::IniFormat);
        file.setIniCodec(QTextCodec::codecForName("UTF-8"));
        file.setValue(section + "/headercount", m_headers.size());
        file.sync();
        for(int i(0); i<m_headers.size(); i++)
        {
            m_headers[i].SaveState(i, section);
        }

    }

    void RestoreState()
    {
        QSettings file("Settings/arm_guistate.ini", QSettings::IniFormat);
        file.setIniCodec(QTextCodec::codecForName("UTF-8"));
        int headerCount = file.value("UI1/headercount").toInt();
        m_headers.resize(headerCount);
        for(int i(0); i<m_headers.size(); i++)
        {
            //m_headers.at(i).SaveState();
            RestoreState(i, m_headers[i]);
        }

    }

    void RestoreState(int i, HeaderState& state)
    {
        QSettings file("Settings/arm_guistate.ini", QSettings::IniFormat);
        file.setIniCodec(QTextCodec::codecForName("UTF-8"));
        QString headerName = "UI1/header" + QString::number(i);
        state.hidden = file.value(headerName + "/hidden").toBool();
        state.size = file.value(headerName + "/size").toInt();
        state.logical_index = file.value(headerName + "/logical_index").toInt();
        state.visual_index = file.value(headerName + "/visual_index").toInt();
        state.name = file.value(headerName + "/name").toString();
    }

    QHeaderViewState(){}



    void
    restoreState(QTableView & view) const
    {
      QHeaderView & headers(*view.horizontalHeader());

      const int max_columns = std::min(headers.count(),
                                       static_cast<int>(m_headers.size()));      

      std::vector<HeaderState> header_state(m_headers);
      std::map<QString,HeaderState *> map;
      for(std::size_t ii = 0; ii < header_state.size(); ++ii)
        map[header_state[ii].name] = &header_state[ii];

      // First set all sections to be hidden and update logical
      // indexes
      for(int li = 0; li < headers.count(); ++li)
      {
        headers.setSectionHidden(li,true);
        std::map<QString,HeaderState *>::iterator it =
          map.find(view.model()->headerData(li,Qt::Horizontal).toString());
        if(it != map.end())
          it->second->logical_index = li;
      }
      
      // Now restore
      for(int vi = 0; vi < max_columns; ++vi)
      {
        HeaderState const & header = header_state[vi];
        const int li = header.logical_index;
        //SSCI_ASSERT_BUG(vi == header.visual_index);
        headers.setSectionHidden(li,header.hidden);
        headers.resizeSection(li,header.size);
        headers.moveSection(headers.visualIndex(li),vi);
      }
//      if(m_sort_indicator_shown)
//        headers.setSortIndicator(m_sort_indicator_section,
//                                 m_sort_order);
    }


    std::vector<HeaderState> m_headers;
//    bool                     m_sort_indicator_shown;
//    int                      m_sort_indicator_section;
//    Qt::SortOrder            m_sort_order; // iff m_sort_indicator_shown
  };

#endif // HEADERVIEWSAVER_H

