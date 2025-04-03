#pragma once

#include <vector>

template <typename T>

/// <summary>
/// It�rateur de variable
/// 
/// Pour �viter un surchage du jeu, on ne peut lire que petite quantit� de variable � chaque frame
/// Cet it�rateur permet de lire les variables par paquet � chaque frame (fen�tre glissante de toute les variables)
/// 
/// Lorsque d'une demande batch est faite, les variables contenue dans cet array sont lues au d�triment des autres
/// 
/// </summary>
class ReadIterator
{
public:

    /// <summary>
    /// Classe interne permetant de cr�er un it�rateur
    /// </summary>
    class Iterator
    {
    public:

        /// <summary>
        /// Cr�ation d'un it�rateur
        /// </summary>
        Iterator(ReadIterator* pParent, bool bIsEnd = false)
        {
            m_pParent = pParent;
            m_nIndex = 0;

            if (bIsEnd)
            {
                m_nIndex = m_pParent->m_nMaxPerFrame;
            }
        }

        /// <summary>
        /// Retourne la valeur � cet instant de la boucle
        /// </summary>
        T &operator*()
        {
            return m_pParent->getValue();
        }

        /// <summary>
        /// Incr�mente l'it�ration
        /// </summary>
        Iterator& operator++()
        {
            m_nIndex++;

            if (m_pParent->m_bIsBatch)
            {
                m_pParent->m_nBatchIndex++;

                if (m_pParent->m_nBatchIndex >= m_pParent->m_batch.size())
                {
                    m_pParent->m_nBatchIndex = 0;
                }
            }
            else
            {
                m_pParent->m_nReadIndex++;

                if (m_pParent->m_nReadIndex >= m_pParent->m_lvars.size())
                {
                    m_pParent->m_nReadIndex = 0;
                }
            }

            return *this;
        }

        /// <summary>
        /// V�rification si la boucle doit s'arr�ter ou non
        /// </summary>
        bool operator!=(const Iterator& other) const
        {
            if (m_pParent->m_bIsBatch)
            {
                return m_pParent->m_batch.size() > 0 && m_nIndex < m_pParent->m_nMaxPerFrame;
            }
            else
            {
                return m_pParent->m_lvars.size() > 0 && m_nIndex < m_pParent->m_nMaxPerFrame;
            }
        }

    private:
        ReadIterator<T>* m_pParent;
        uint16_t m_nIndex;
    };

    /// <summary>
    /// Cr�ation de l'it�rateur de lecture de variable
    /// </summary>
    explicit ReadIterator(uint16_t nMaxPerFrame, uint16_t nBatchDuration)
    {
        m_nMaxPerFrame = nMaxPerFrame;
        m_nBatchDuration = nBatchDuration;
        m_nReadIndex = 0;
        m_nBatchIndex = 0;
        m_bIsBatch = false;
    }

    /// <summary>
    /// Rien � supprimer ici, le destructeur reste � default
    /// </summary>
    virtual ~ReadIterator() = default;

    /// <summary>
    /// Cr�ation de l'it�ration "begin" appel� � chaque nouvelle boucle
    /// </summary>
    Iterator begin()
    {
        if (m_bIsBatch)
        {
            m_nBatchHint++;

            if (m_nBatchHint > m_nBatchDuration)
            {
                m_bIsBatch = false;
            }
        }
        return ReadIterator::Iterator(this);
    }

    /// <summary>
    /// Cr�ation de l'it�ration "end" appel� � chaque tour de boucle (permet de v�rifier si la boucle doit s'arr�ter)
    /// </summary>
    /// <returns></returns>
    Iterator end()
    {
        return ReadIterator::Iterator(this, true);
    }

    /// <summary>
    /// Ajoute une variable � la liste
    /// </summary>
    void push_back(T value)
    {
        m_lvars.push_back(value);
    }

    /// <summary>
    /// Retourne la liste des variables
    /// </summary>
    std::vector<T> &getValues()
    {
        return m_lvars;
    }

    /// <summary>
    /// Ajout un batch de lecture (lecture prioritaire)
    /// </summary>
    void setBatch(const std::vector<T>& batch)
    {
        if (batch.size() > 0)
        {
            m_nBatchHint = 0;
            m_bIsBatch = true;
            m_nBatchIndex = 0;
            m_batch = batch;
        }
    }

private:
    uint16_t m_nReadIndex;
    uint16_t m_nBatchIndex;
    uint16_t m_nMaxPerFrame;
    uint16_t m_nBatchDuration;
    uint16_t m_nBatchHint;
    bool m_bIsBatch;

    std::vector<T> m_lvars;
    std::vector<T> m_batch;

    /// <summary>
    /// Returne la valeur � cet instant de la boucle
    /// </summary>
    T& getValue()
    {
        if (m_bIsBatch)
        {
            return m_batch[m_nBatchIndex];
        }
        else
        {
            return m_lvars[m_nReadIndex];
        }
    }
};

